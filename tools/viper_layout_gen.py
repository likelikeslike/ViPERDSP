#!/usr/bin/env python3
"""ViperParamsLayout codegen via libclang AST + C++ offsetof probe.

Single source of truth: `ViPERDSP/include/ViPERParams.h`. Adds zero
manual maintenance — the script enumerates structs and fields by
walking the AST, then runs a tiny generated C++ probe to obtain
byte-perfect `offsetof()`/`sizeof()` values from the SAME compiler the
app is built with.

- Usage examples

ViPER4Android:

    python ViPERDSP/tools/viper_layout_gen.py \
        --header          ViPERDSP/include/ViPERParams.h \
        --include_dir     ViPERDSP/include \
        --lang            kotlin \
        --package         com.llsl.viper4android.viper \
        --output          ViperParamsLayout.kt \
        --generator_path  ViPERDSP/tools/viper_layout_gen.py

ViPER4Mac:

    python ViPERDSP/tools/viper_layout_gen.py \
        --header          ViPERDSP/include/ViPERParams.h \
        --include_dir     ViPERDSP/include \
        --lang            swift \
        --output          ViperParamsLayout.swift \
        --generator_path  ViPERDSP/tools/viper_layout_gen.py

ViPER4Windows:

    python ViPERDSP/tools/viper_layout_gen.py \
        --header          ViPERDSP/include/ViPERParams.h \
        --include_dir     ViPERDSP/include \
        --lang            dart \
        --output          viper_params_layout.dart \
        --generator_path  ViPERDSP/tools/viper_layout_gen.py
"""

from __future__ import annotations

import argparse
import dataclasses
import subprocess
import sys
import tempfile
from collections.abc import Callable, Iterable
from pathlib import Path

import clang.cindex
from clang.cindex import Cursor, CursorKind


@dataclasses.dataclass
class Field:
    """One field inside a viper::*Params struct."""

    name: str
    type_spelling: str  # raw C++ type as written, e.g. "uint32_t",
    array_element: str | None  # for std::array<T, N>: "T". None otherwise.
    array_count: int | None  # for std::array<T, N>: N. None otherwise.


@dataclasses.dataclass
class StructDef:
    """One viper::*Params struct."""

    cpp_name: str  # e.g. "BassParams"
    layout_name: str  # public layout name, e.g. "Bass"
    fields: list[Field]


@dataclasses.dataclass
class LangConfig:
    """Per-language emission settings. Letting the CLI configure these
    keeps one Python tool serving every app — V4M, V4A, V4W — without
    per-app forks.

    `style`:
      - "nested": Swift / Kotlin pattern. Sub-structs as nested
        enum/object inside the root, accessed as
        `ViperParamsLayout.MasterLimiter.threshold`.
      - "flat_classes": Dart pattern. Sub-structs as separate
        top-level classes (Dart forbids true nesting), accessed as
        `MasterLimiterLayout.threshold`.
    """

    name: str  # "swift" / "kotlin" / "dart" — for diagnostics
    style: str  # "nested" | "flat_classes"
    file_header: str  # opening lines: package / import / banner
    open_root: str  # block-opening line for the root container
    open_substruct: str  # block-opening for a sub-struct (uses {layout_name})
    decl_root: str  # constant declaration at root scope (uses {name}/{value})
    decl_substruct: str  # constant declaration inside a sub-struct
    close_block: str  # block-close line
    array_suffix: str  # `Len` / `_LEN`
    name_case: Callable[[str], str]  # casing for C++-derived const names.
    indent: str

    def at(self, level: int, text: str) -> str:
        return f"{self.indent * level}{text}"


def discover_stdlib_includes(compiler: str) -> list[str]:
    result = subprocess.run(
        [compiler, "-E", "-x", "c++", "-v", "-"],
        input="",
        capture_output=True,
        text=True,
    )
    paths: list[str] = []
    in_search_list = False
    for line in result.stderr.splitlines():
        if line.startswith("#include <...>"):
            in_search_list = True
            continue
        if line.startswith("End of search list"):
            in_search_list = False
            continue
        if in_search_list and line.startswith(" "):
            paths.append(line.strip().split(" (")[0])
    return paths


def parse_header(
    header: Path,
    include_dirs: list[Path],
    compiler: str,
) -> dict[str, StructDef]:
    """Parse `header`, return all structs in the `viper` namespace."""
    index = clang.cindex.Index.create()
    args = ["-std=c++17", "-x", "c++"]
    for inc in include_dirs:
        args.extend(["-I", str(inc)])
    for inc in discover_stdlib_includes(compiler):
        args.extend(["-isystem", inc])
    tu = index.parse(str(header), args=args)
    if not tu:
        raise SystemExit(f"libclang failed to parse {header}")
    diagnostics = list(tu.diagnostics)
    fatal = [d for d in diagnostics if d.severity >= clang.cindex.Diagnostic.Error]
    if fatal:
        for d in fatal:
            print(f"clang error: {d.spelling} at {d.location}", file=sys.stderr)
        raise SystemExit("AST parse failed; fix C++ errors before codegen")

    structs: dict[str, StructDef] = {}

    def visit(cursor: Cursor, in_viper_ns: bool) -> None:
        if cursor.kind == CursorKind.NAMESPACE:
            for c in cursor.get_children():
                visit(c, in_viper_ns or cursor.spelling == "viper")
            return
        if not in_viper_ns:
            for c in cursor.get_children():
                visit(c, False)
            return
        if cursor.kind in (CursorKind.STRUCT_DECL, CursorKind.CLASS_DECL):
            if cursor.is_definition() and cursor.spelling:
                fields = list(extract_fields(cursor))
                structs[cursor.spelling] = StructDef(
                    cpp_name=cursor.spelling,
                    layout_name=strip_params_suffix(cursor.spelling),
                    fields=fields,
                )

    visit(tu.cursor, in_viper_ns=False)
    return structs


def strip_params_suffix(name: str) -> str:
    return name.removesuffix("Params")


def strip_namespace(s: str) -> str:
    return s.split("::")[-1]


def extract_fields(cursor: Cursor) -> Iterable[Field]:
    """Yield each field of a struct, including std::array<T,N>."""
    for c in cursor.get_children():
        if c.kind != CursorKind.FIELD_DECL:
            continue
        t = c.type
        ts = t.spelling
        elem, count = parse_std_array(t)
        yield Field(
            name=c.spelling,
            type_spelling=ts,
            array_element=elem,
            array_count=count,
        )


def parse_std_array(t) -> tuple[str | None, int | None]:
    """If `t` is `std::array<T, N>`, return (T, N). Else (None, None)."""
    s = t.spelling
    if not s.startswith("std::array<") and not s.startswith("std::__1::array<"):
        return (None, None)
    n_args = t.get_num_template_arguments()
    if n_args < 2:
        return (None, None)
    elem_t = t.get_template_argument_type(0)
    elem_spelling = strip_namespace(elem_t.spelling)
    inner = s.split("<", 1)[1].rsplit(">", 1)[0]
    parts = [p.strip() for p in inner.rsplit(",", 1)]
    if len(parts) != 2 or not parts[1].isdigit():
        return (elem_spelling, None)
    return (elem_spelling, int(parts[1]))


def probe_layout(
    structs: dict[str, StructDef],
    header: Path,
    include_dirs: list[Path],
    compiler: str,
) -> dict[str, dict]:
    """Compile a probe binary that prints offsetof()/sizeof() for every
    field and struct, parse the output.

    Returns: `{struct_name: {"SIZE": int, fields: {name: offset}}}`.
    """
    probe_src = build_probe_source(structs, header)
    with tempfile.TemporaryDirectory() as tmp:
        src_path = Path(tmp) / "probe.cpp"
        bin_path = Path(tmp) / "probe"
        src_path.write_text(probe_src)
        cmd = [compiler, "-std=c++17"]
        for inc in include_dirs:
            cmd.extend(["-I", str(inc)])
        cmd.extend([str(src_path), "-o", str(bin_path)])
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            print("probe compile failed:", file=sys.stderr)
            print(result.stderr, file=sys.stderr)
            raise SystemExit(1)
        result = subprocess.run([str(bin_path)], capture_output=True, text=True)
        if result.returncode != 0:
            raise SystemExit("probe runtime failed: " + result.stderr)
        return parse_probe_output(result.stdout)


def build_probe_source(structs: dict[str, StructDef], header: Path) -> str:
    """Generate a C++ source file that prints offsetof()/sizeof() for every field and struct."""
    lines = [
        "#include <cstddef>",
        "#include <cstdio>",
        f'#include "{header.name}"',
        "int main() {",
    ]
    for s in structs.values():
        lines.append(
            f'    std::printf("STRUCT\\t{s.cpp_name}\\t%zu\\n", sizeof(viper::{s.cpp_name}));'
        )
        for f in s.fields:
            lines.append(
                f'    std::printf("FIELD\\t{s.cpp_name}\\t{f.name}\\t%zu\\n", '
                f"offsetof(viper::{s.cpp_name}, {f.name}));"
            )
    lines.append("    return 0;")
    lines.append("}")
    return "\n".join(lines) + "\n"


def parse_probe_output(out: str) -> dict[str, dict]:
    """Parse the probe binary's stdout into `{struct: {"SIZE": int, "fields": {name: offset}}}`."""
    layout: dict[str, dict] = {}
    for line in out.strip().splitlines():
        parts = line.split("\t")
        if parts[0] == "STRUCT":
            _, name, size = parts
            layout.setdefault(name, {"fields": {}})["SIZE"] = int(size)
        elif parts[0] == "FIELD":
            _, struct_name, field_name, offset = parts
            layout.setdefault(struct_name, {"fields": {}})["fields"][field_name] = int(
                offset
            )
    return layout


def make_kotlin_config(
    generator_path: str,
    package: str,
    indent_size: int,
) -> LangConfig:
    indent = " " * indent_size
    return LangConfig(
        name="kotlin",
        style="nested",
        file_header=(
            f"// AUTO-GENERATED by {generator_path}\n"
            "// DO NOT EDIT MANUALLY. Regenerate this file after\n"
            "// any change to `ViPERDSP/include/ViPERParams.h`.\n"
            "//\n"
            "// Mirrors viper::ViPERParams byte layout sent through typed\n"
            "// parameter dispatch. Field offsets are derived from the same C++\n"
            "// header the AIDL HAL consumer compiles against — Kotlin and C++\n"
            "// cannot disagree on struct shape.\n"
            f"package {package}\n"
        ),
        open_root="object ViperParamsLayout {",
        open_substruct=f"{indent}object {{layout_name}} {{{{",
        decl_root=f"{indent}const val {{name}}: Int = {{value}}",
        decl_substruct=f"{indent * 2}const val {{name}}: Int = {{value}}",
        close_block=f"{indent}}}",
        array_suffix="_LEN",
        name_case=to_screaming_snake,
        indent=indent,
    )


def make_swift_config(generator_path: str, indent_size: int) -> LangConfig:
    indent = " " * indent_size
    return LangConfig(
        name="swift",
        style="nested",
        file_header=(
            f"// AUTO-GENERATED by {generator_path}\n"
            "// DO NOT EDIT BY HAND. Regenerate by running `make layout` after\n"
            "// any change to ViPERDSP/include/ViPERParams.h.\n"
            "//\n"
            "// Mirrors viper::ViPERParams byte layout for the typed-params\n"
            "// marshaler in ViPER4Mac. Field offsets are derived from the same C++\n"
            "// header the bridge compiles against — Swift and C++ cannot disagree\n"
            "// on struct shape.\n"
            "import Foundation\n"
        ),
        open_root="enum ViperParamsLayout {",
        open_substruct=f"{indent}enum {{layout_name}} {{{{",
        decl_root=f"{indent}static let {{name}}: Int = {{value}}",
        decl_substruct=f"{indent * 2}static let {{name}}: Int = {{value}}",
        close_block=f"{indent}}}",
        array_suffix="Len",
        name_case=_to_camel,
        indent=indent,
    )


def make_dart_config(generator_path: str, indent_size: int) -> LangConfig:
    indent = " " * indent_size
    return LangConfig(
        name="dart",
        style="flat_classes",
        file_header=(
            f"// AUTO-GENERATED by {generator_path}\n"
            "// DO NOT EDIT BY HAND. Regenerate by running `make layout` after\n"
            "// any change to ViPERDSP/include/ViPERParams.h.\n"
            "//\n"
            "// Mirrors viper::ViPERParams byte layout for the shm producer\n"
            "// in ViPER4Windows's Flutter UI. Field offsets are derived from\n"
            "// the same C++ header the APO compiles against — Dart and C++\n"
            "// cannot disagree on struct shape.\n"
            "// ignore_for_file: constant_identifier_names\n"
        ),
        open_root="abstract final class ViperParamsLayout {",
        open_substruct="abstract final class {layout_name}Layout {{",
        decl_root=f"{indent}static const int {{name}} = {{value}};",
        decl_substruct=f"{indent}static const int {{name}} = {{value}};",
        close_block="}",
        array_suffix="Len",
        name_case=_to_camel,
        indent=indent,
    )


def emit(
    structs: dict[str, StructDef],
    layout: dict[str, dict],
    cfg: LangConfig,
) -> str:
    """Render the layout file for the given language config. Two
    rendering styles supported:

      - "nested" (Swift, Kotlin): root container holds the sub-struct
        offsets at top level and the per-sub-struct field offsets
        nested inside as enum/object blocks.
      - "flat_classes" (Dart): root container holds only the
        sub-struct offsets; per-sub-struct field offsets live in
        sibling top-level classes named `{layout_name}Layout`.
    """
    lines = [cfg.file_header.rstrip("\n"), ""]
    lines.append(cfg.open_root)

    root = structs["ViPERParams"]
    root_layout = layout["ViPERParams"]
    lines.append(cfg.at(1, "// Root struct: viper::ViPERParams"))
    lines.append(cfg.decl_root.format(name="SIZE", value=root_layout["SIZE"]))
    for f in root.fields:
        offset = root_layout["fields"][f.name]
        lines.append(cfg.decl_root.format(name=cfg.name_case(f.name), value=offset))

    if cfg.style == "nested":
        for s in structs.values():
            if s.cpp_name == "ViPERParams":
                continue
            lines.append("")
            lines.append(cfg.open_substruct.format(layout_name=s.layout_name))
            struct_layout = layout[s.cpp_name]
            lines.append(
                cfg.decl_substruct.format(name="SIZE", value=struct_layout["SIZE"])
            )
            for f in s.fields:
                offset = struct_layout["fields"][f.name]
                lines.append(
                    cfg.decl_substruct.format(name=cfg.name_case(f.name), value=offset)
                )
                if f.array_count is not None:
                    const_name = cfg.name_case(f.name) + cfg.array_suffix
                    lines.append(
                        cfg.decl_substruct.format(name=const_name, value=f.array_count)
                    )
            lines.append(cfg.close_block)
        lines.append("}")
    elif cfg.style == "flat_classes":
        lines.append(cfg.close_block)
        for s in structs.values():
            if s.cpp_name == "ViPERParams":
                continue
            lines.append("")
            lines.append(cfg.open_substruct.format(layout_name=s.layout_name))
            struct_layout = layout[s.cpp_name]
            lines.append(
                cfg.decl_substruct.format(name="SIZE", value=struct_layout["SIZE"])
            )
            for f in s.fields:
                offset = struct_layout["fields"][f.name]
                lines.append(
                    cfg.decl_substruct.format(name=cfg.name_case(f.name), value=offset)
                )
                if f.array_count is not None:
                    const_name = cfg.name_case(f.name) + cfg.array_suffix
                    lines.append(
                        cfg.decl_substruct.format(name=const_name, value=f.array_count)
                    )
            lines.append(cfg.close_block)
    else:
        raise SystemExit(f"unknown emit style: {cfg.style}")

    return "\n".join(lines) + "\n"


def to_screaming_snake(name: str) -> str:
    out: list[str] = []
    for i, ch in enumerate(name):
        if ch == "_":
            out.append("_")
            continue
        if ch.isupper() and i > 0 and out and out[-1] != "_":
            out.append("_")
        out.append(ch.upper())
    return "".join(out)


def _to_camel(name: str) -> str:
    parts = name.split("_")
    return parts[0] + "".join(p.capitalize() for p in parts[1:])


def default_indent_size(lang: str) -> int:
    return 4 if lang == "kotlin" else 2


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--header", required=True, type=Path)
    parser.add_argument("--include_dir", action="append", required=True, type=Path)
    parser.add_argument("--lang", choices=["swift", "kotlin", "dart"], required=True)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument(
        "--compiler",
        default="clang++",
        help="C++ compiler used to build the probe (default clang++)",
    )
    parser.add_argument(
        "--package", default=None, help="Kotlin package (required when --lang=kotlin)"
    )
    parser.add_argument(
        "--generator_path",
        default=None,
        help="Path string to record in the file header banner. "
        "Defaults to ViPERDSP/tools/viper_layout_gen.py.",
    )
    parser.add_argument(
        "--struct_filter",
        default=None,
        help="If set, only emit structs whose name matches this substring (spike mode)",
    )
    parser.add_argument(
        "--indent",
        type=int,
        default=None,
        help="Spaces per indentation level. Defaults to 2 for Swift/Dart, 4 for Kotlin.",
    )
    args = parser.parse_args()

    structs = parse_header(args.header, args.include_dir, compiler=args.compiler)
    if args.struct_filter:
        wanted = {
            k: v
            for k, v in structs.items()
            if args.struct_filter in k or k == "ViPERParams"
        }
        if "ViPERParams" in wanted:
            root = wanted["ViPERParams"]
            kept_field_types = {s.cpp_name for s in wanted.values()}
            wanted["ViPERParams"] = StructDef(
                cpp_name=root.cpp_name,
                layout_name=root.layout_name,
                fields=[
                    f
                    for f in root.fields
                    if any(t in f.type_spelling for t in kept_field_types)
                ],
            )
        structs = wanted

    if not structs:
        raise SystemExit("No structs found in viper namespace")
    if "ViPERParams" not in structs:
        raise SystemExit("Root viper::ViPERParams not found")

    layout = probe_layout(
        structs, args.header, args.include_dir, compiler=args.compiler
    )

    gen_path = args.generator_path or "ViPERDSP/tools/viper_layout_gen.py"
    indent_size = (
        args.indent if args.indent is not None else default_indent_size(args.lang)
    )
    if indent_size < 0:
        raise SystemExit("--indent must be zero or greater")

    if args.lang == "kotlin":
        if not args.package:
            raise SystemExit("--package is required for --lang=kotlin")
        cfg = make_kotlin_config(gen_path, args.package, indent_size)
    elif args.lang == "swift":
        cfg = make_swift_config(gen_path, indent_size)
    elif args.lang == "dart":
        cfg = make_dart_config(gen_path, indent_size)
    else:
        raise SystemExit(f"lang {args.lang} not supported")

    out = emit(structs, layout, cfg)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(out)
    print(f"wrote {args.output} ({len(out.splitlines())} lines)")


if __name__ == "__main__":
    main()
