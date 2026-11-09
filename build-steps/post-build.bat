if not defined MGR_PLUGIN_SDK_GAME_PATH (
    echo Game directory was not specified. Ignoring copying the .asi script into the game folder.
    exit /b 0
)

if not exist "%MGR_PLUGIN_SDK_GAME_PATH%"  (
    echo Game directory was not specified. Ignoring copying the .asi script into the game folder.
    exit /b 0
)

copy /Y "%~1" "%MGR_PLUGIN_SDK_GAME_PATH%/scripts/"
if errorlevel 1  (
    echo Failed to copy the .asi script into the game folder.
    exit /b 1
)

echo Successfully copied the .asi script into the game folder.