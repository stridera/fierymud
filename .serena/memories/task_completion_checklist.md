# Task Completion Checklist

## Before Marking Tasks Complete

### 1. Build Verification
```bash
cmake --build build
```
- Ensure clean compilation with no errors
- Address any warnings if possible

### 2. Testing
```bash
./build/tests
```
- Run all unit tests and ensure they pass
- Add new tests for new functionality
- Verify existing functionality still works

### 3. Code Formatting
- Code automatically follows .clang-format rules (LLVM style, 120 char limit)
- Check that added code follows project conventions

### 4. Functionality Verification
- Test the actual feature/fix works as intended
- Verify no regressions in existing functionality
- Check edge cases and error conditions

## Task Completion Evidence
- **Compilation**: Clean build with no errors
- **Tests**: All tests pass, new tests added if appropriate  
- **Functionality**: Feature works as specified
- **Style**: Code follows project conventions
- **Integration**: No conflicts with existing systems

## Common Issues to Check
- **Memory Management**: Proper smart pointer usage
- **Error Handling**: `std::expected` for error returns
- **Thread Safety**: Consider concurrent access if applicable
- **Performance**: No obvious performance regressions