# Claude Code Guidelines for M5StickC Plus 1.1 Development

## Code Style Rules

### Emoji Usage
- **NEVER use emojis in Arduino C++ code files (.ino, .cpp, .h)**
- Emojis cause compilation errors and encoding issues in the Arduino IDE
- Use plain ASCII text only for all code comments and strings displayed on the M5StickC screen
- Example: Use "Temperature: 25C" instead of "Temperature: 25°C" or "🌡️ 25C"

### Commit Messages
- Git commit messages MAY include emojis (they are allowed in commit messages)
- Code files themselves must NOT include emojis

## General Guidelines
- Keep code compatible with Arduino IDE and PlatformIO
- Use English for code comments
- Test all code before committing
