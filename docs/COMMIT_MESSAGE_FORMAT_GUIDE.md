# Commit Message Conventions

This project uses a structured commit message format to ensure a readable, searchable history and to support automated changelog generation.

## Message Structure

The commit message should follow this pattern:
`type(scope): description`

### 1. Types
The **type** tells us what kind of change you are making:

| Type | Purpose |
| :--- | :--- |
| **feat** | A new feature or functionality |
| **fix** | A bug fix or correction |
| **docs** | Documentation changes only (README, etc.) |
| **chore** | Maintenance (logging, dependencies, build tasks) |
| **refacto** | Code changes that neither fix a bug nor add a feature |
| **test** | Adding or updating tests |

### 2. Scopes
The **scope** defines which part of the system is affected. Examples used in this project include:
* `tools`: Individual tool implementations (e.g., lasso, brush).
* `selection`: Logic related to selection states and menus.
* `core`: Shared base classes, formatting, and engine logic.
* `global`: Metadata, authoring info, or project-wide settings.
* `_`: Used for broad changes that don't fit a specific module.

### 3. Subject Line Rules
* **Use the imperative mood:** "add" instead of "added" or "adds".
* **Lower case:** Do not capitalize the first letter of the subject.
* **No period:** Do not end the subject line with a trailing period.
* **Be concise:** Keep the subject line under 72 characters if possible.
