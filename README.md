# QtWidgetsTemplate

This repository is both a GitHub template repository and a
[Copier](https://copier.readthedocs.io/) template for Qt 6 Widgets applications and
libraries.

GitHub creates the repository first so that the generated project retains the
**generated from Dingola/QtWidgetsTemplate** relationship. Copier then renders all
project-specific names and settings into that repository.

## Create a project

### 1. Create the repository on GitHub

1. Open this repository on GitHub.
2. Select **Use this template** and then **Create a new repository**.
3. Enter the final repository name, for example `QtRecordParser`.
4. Select the owner and visibility and create the repository.

Do not rename the local project directory after cloning unless you also pass an explicit
`-ProjectName` to the initialization script.

### 2. Install local prerequisites

Install Git, Python, and [pipx](https://pipx.pypa.io/), then install Copier:

```powershell
pipx install copier
copier --version
```

Copier 9.0 or newer is required.

### 3. Clone and initialize the repository

```powershell
git clone https://github.com/<OWNER>/QtRecordParser.git
cd QtRecordParser
.\initialize.ps1
```

The script uses the repository directory name as the CMake project name. It validates
the Git working tree, runs Copier against `gh:Dingola/QtWidgetsTemplate`, and removes
the bootstrap-only files after successful generation.

To use a different CMake name:

```powershell
.\initialize.ps1 -ProjectName QtRecordParser
```

To test an unreleased template revision:

```powershell
.\initialize.ps1 -VcsRef HEAD
```

Use released template versions for normal projects. `HEAD` is intended for template
development and testing.

### 4. Review and commit the generated project

```powershell
git status
git diff
git add .
git commit -m "Initialize project from QtWidgetsTemplate"
git push
```

The GitHub template relationship remains intact because the repository was originally
created with **Use this template**.

## Update a generated project

Generated projects contain `.copier-answers.yml`. Do not edit that file manually.
From a clean Git working tree, update the project with:

```powershell
copier update
```

Review all changes and resolve possible conflicts before committing the update.

## Template maintenance

The repository root contains the bootstrap:

```text
.
|-- copier.yml
|-- initialize.ps1
|-- README.md
`-- template/
    |-- README.md.jinja
    |-- Dockerfile.jinja
    |-- CMakeLists.txt.jinja
    |-- QT_Project/
    |-- QT_Project_Tests/
    |-- Scripts/
    `-- .github/
```

All files that must appear in generated projects belong under `template/`. Files whose
contents contain Copier expressions use the `.jinja` suffix. Copier removes this suffix
when rendering.

After changing the template:

1. Generate a test project into a separate directory.
2. Configure, build, and test the generated project.
3. Commit the template changes.
4. Create a PEP 440-compatible Git tag such as `1.0.0` or `1.1.0`.
5. Push the commit and tag to GitHub.

Example local generation:

```powershell
copier copy --vcs-ref HEAD . ..\QtWidgetsTemplateSmokeTest
```

Example release:

```powershell
git tag 1.0.0
git push origin main
git push origin 1.0.0
```

Copier selects released tags for normal generation and uses them to calculate future
project updates.

## Migration layout implemented in this repository

The previous project files were moved as follows:

| Previous path | Copier source path |
| --- | --- |
| `.github/` | `template/.github/` |
| `CMake/` | `template/CMake/` |
| `Configs/` | `template/Configs/` |
| `QT_Project/` | `template/QT_Project/` |
| `QT_Project_Tests/` | `template/QT_Project_Tests/` |
| `Scripts/` | `template/Scripts/` |
| `ThirdParty/` | `template/ThirdParty/` |
| `.gitignore` | `template/.gitignore` |
| `CMakeLists.txt` | `template/CMakeLists.txt.jinja` |
| `Dockerfile` | `template/Dockerfile.jinja` |
| Project `README.md` | `template/README.md.jinja` |
| `QT_Project/main.cpp` | `template/QT_Project/main.cpp.jinja` |
| `QT_Project_Tests/main.cpp` | `template/QT_Project_Tests/main.cpp.jinja` |
| `QT_Project/Headers/ApiMacro.h` | `template/QT_Project/Headers/ApiMacro.h.jinja` |

The root `.gitignore` is intentionally retained for template development. A copy under
`template/` becomes the generated project's `.gitignore`.

The following root files are bootstrap-only:

- `.github/workflows/validate_template.yml` generates, builds, and tests a smoke project.
- `copier.yml` defines questions, validation, and `template/` as the Copier source.
- `initialize.ps1` safely initializes a repository created through GitHub.
- `README.md` documents template usage and maintenance.
- `template/` contains the complete generated-project source.

After successful generation, `initialize.ps1` removes `copier.yml`, `initialize.ps1`,
`template/`, and the template-validation workflow from the generated repository. The
rendered project README replaces the bootstrap README. `.copier-answers.yml` remains
for future updates.

## Publishing this migration to the existing GitHub template

The repository is already hosted on GitHub, so no new template repository is required.
Publish the migration with these steps:

1. Review the complete move and all new files with `git status` and `git diff`.
2. Commit the migration on the template repository's default branch.
3. Push that branch to `Dingola/QtWidgetsTemplate`.
4. In GitHub repository settings, confirm that **Template repository** is still enabled.
5. Create and push the first Copier-compatible release tag, for example `1.0.0`.
6. Create a temporary repository through **Use this template** and run
   `.\initialize.ps1` as an end-to-end verification.

Suggested commands:

```powershell
git add .
git commit -m "Convert QtWidgetsTemplate to a Copier template"
git push origin main
git tag 1.0.0
git push origin 1.0.0
```

Replace `main` if the repository uses a different default branch. Do not create the
release tag until generation, configuration, compilation, and tests have succeeded.
