cmake_minimum_required(VERSION 3.16)
project(ViPERDSP LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(VIPERDSP_SOURCES
    # FFT
    viper/utils/pffft.c

    # Engine
    viper/ViPER.cpp

    # Effects
    viper/effects/AnalogX.cpp
    viper/effects/ColorfulMusic.cpp
    viper/effects/Convolver.cpp
    viper/effects/Cure.cpp
    viper/effects/DiffSurround.cpp
    viper/effects/DynamicEQ.cpp
    viper/effects/DynamicSystem.cpp
    viper/effects/FETCompressor.cpp
    viper/effects/IIRFilter.cpp
    viper/effects/LUFSTargeting.cpp
    viper/effects/MultibandCompressor.cpp
    viper/effects/PlaybackGain.cpp
    viper/effects/Reverberation.cpp
    viper/effects/SoftwareLimiter.cpp
    viper/effects/SpeakerCorrection.cpp
    viper/effects/SpectrumExtend.cpp
    viper/effects/StereoImager.cpp
    viper/effects/TubeSimulator.cpp
    viper/effects/VHE.cpp
    viper/effects/ViPERBass.cpp
    viper/effects/ViPERBassMono.cpp
    viper/effects/ViPERClarity.cpp
    viper/effects/ViPERDDC.cpp

    # Utils
    viper/utils/AdaptiveBuffer.cpp
    viper/utils/Biquad.cpp
    viper/utils/CAllpassFilter.cpp
    viper/utils/CCombFilter.cpp
    viper/utils/CRevModel.cpp
    viper/utils/Crossfeed.cpp
    viper/utils/DepthSurround.cpp
    viper/utils/DynamicBass.cpp
    viper/utils/FIR.cpp
    viper/utils/Harmonic.cpp
    viper/utils/HiFi.cpp
    viper/utils/HighShelf.cpp
    viper/utils/IIR_1st.cpp
    viper/utils/IIR_NOrder_BW_BP.cpp
    viper/utils/IIR_NOrder_BW_LH.cpp
    viper/utils/MinPhaseIIRCoeffs.cpp
    viper/utils/MultiBiquad.cpp
    viper/utils/NoiseSharpening.cpp
    viper/utils/PassFilter.cpp
    viper/utils/PConvSingle.cpp
    viper/utils/PolesFilter.cpp
    viper/utils/Polyphase.cpp
    viper/utils/Stereo3DSurround.cpp
    viper/utils/Subwoofer.cpp
    viper/utils/TimeConstDelay.cpp
    viper/utils/WaveBuffer.cpp
)

add_library(ViPERDSP STATIC ${VIPERDSP_SOURCES})

target_include_directories(ViPERDSP PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(ViPERDSP PRIVATE -O3 -flto)

if(ANDROID)
    target_link_libraries(ViPERDSP log)
endif()
