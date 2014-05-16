# SDL_MIXER_FOUND
# SDL_MIXER_INCLUDE_DIR
# SDL_MIXER_LIBRARY

if (WIN32)
  find_path (SDL_MIXER_INCLUDE_DIR SDL/SDL_mixer.h
             ${SDL_ROOT_PATH}/include
             ${PROJECT_SOURCE_DIR}/include
             doc "The directory where SDL_mixer.h header resides")
  find_library (SDL_MIXER_LIBRARY
                NAMES SDL_mixer
                PATHS
                ${PROJECT_SOURCE_DIR}/lib
                DOC "The SDL_mixer library")
else (WIN32)
  find_path (SDL_MIXER_INCLUDE_DIR SDL/SDL_mixer.h
             ~/include
             /usr/include
             /usr/local/include)
  find_library (SDL_MIXER_LIBRARY SDL_mixer
                ~/lib
                /usr/lib
                /usr/local/lib)
endif (WIN32)

if (SDL_MIXER_INCLUDE_DIR)
  set (SDL_MIXER_FOUND 1 CACHE STRING "Set to 1 if SDL_mixer is found, 0 otherwise")
else (SDL_MIXER_INCLUDE_DIR)
  set (SDL_MIXER_FOUND 0 CACHE STRING "Set to 1 if SDL_mixer is found, 0 otherwise")
endif (SDL_MIXER_INCLUDE_DIR)

mark_as_advanced (SDL_MIXER_FOUND)

