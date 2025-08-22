# Safety Rules

## CRITICAL RULES - NEVER VIOLATE THESE:

### 1. File System Safety
- **NEVER** use `rm -rf` or force deletion commands
- **NEVER** modify files outside the current project directory
- **NEVER** access or modify system files (/etc, /usr, /System, etc.)
- **ALWAYS** create backups before modifying important files

### 2. Git Safety
- **NEVER** force push to main/master branches
- **NEVER** rewrite published git history
- **NEVER** delete remote branches without explicit permission
- **ALWAYS** create feature branches for changes
- **ALWAYS** use descriptive commit messages

### 3. Security Rules
- **NEVER** commit passwords, API keys, or secrets
- **NEVER** expose sensitive information in logs
- **NEVER** disable security features
- **NEVER** run commands with sudo/admin privileges
- **ALWAYS** use environment variables for sensitive data

### 4. Network Safety
- **NEVER** perform port scanning or network attacks
- **NEVER** access unauthorized external services
- **NEVER** download executables from untrusted sources
- **ALWAYS** verify HTTPS certificates

### 5. Resource Management
- **NEVER** create infinite loops or resource-intensive operations
- **NEVER** consume excessive disk space (>1GB)
- **NEVER** spawn more than 10 concurrent processes
- **ALWAYS** clean up temporary files

## BEST PRACTICES:

### Development Workflow
1. Always run tests before committing
2. Use semantic versioning for releases
3. Follow existing code style and conventions
4. Document all significant changes
5. Review changes before finalizing
6. Don't give up and take a simple approach when a better path exists

### Error Handling
1. Catch and log all errors appropriately
2. Never suppress error messages
3. Fail gracefully with helpful error messages
4. Create rollback plans for risky operations

### Communication
1. Log all significant actions
2. Create summary reports after task completion
3. Highlight any issues or concerns
4. Ask for clarification if tasks are ambiguous

## ALLOWED ACTIONS:

### Code Operations
- Read and analyze source code
- Create new files in the project directory
- Modify existing project files
- Run linters and formatters
- Execute test suites
- Build and compile code

### Git Operations
- Stay within the current branch unless told otherwise.
- Stage and commit changes
- View git history and diffs
- Create pull requests
- Tag releases

### Documentation
- Generate documentation
- Update README files
- Create changelog entries
- Add code comments
- Write tutorials or guides

### Dependency Management
- Install project dependencies
- Update package versions (following semver)
- Audit for security vulnerabilities
- Generate dependency graphs

## FORBIDDEN ACTIONS:

1. Accessing user's personal files
2. Modifying IDE or editor configurations
3. Changing system settings
4. Installing global packages without permission
5. Accessing credentials or secrets
6. Making external API calls without explicit permission
7. Modifying production configurations
8. Deleting user data or backups

## EXECUTION LIMITS:

- Maximum execution time: 1 hour per session
- Maximum file size for creation: 10MB
- Maximum number of files to modify: 100
- Maximum commits per session: 20

Remember: When in doubt, choose the safer option or log the concern for human review.

