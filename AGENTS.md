Lanzhou University Development Agent Guide
## Project Overview

**Project Name**: Rove Lanzhou University - Campus Growth Simulation System  
**Tech Stack**: C++17, QT 5.14.2, SQLite3, CMake  
**Project Type**: Standalone Desktop Application  
**Code Target**: ≥1800 lines of C++ code

## Technical Specifications

### Development Environment

- **QT Version**: 5.14.2 (strictly matches teacher's requirement)

- **C++ Standard**: C++17

- **Compiler**: MSVC 2017/GCC 7.3+ (compatible with QT 5.14.2)

- **Database**: SQLite3

- **UI Framework**: QT Widgets

- **Architecture**: Standalone, no network functionality


### Code Standards

cpp

// 1. EVERY CLASS AND METHOD MUST HAVE DETAILED COMMENTARY FOR CODE EXPLANATION
// 2. Exception-safe design
// 3. Comment complex algorithms and business logic thoroughly

### COMMENTARY REQUIREMENTS (FOR TEACHER EXPLANATION)

- **Class Comments**: Purpose, responsibilities, design patterns used

- **Method Comments**: Functionality, parameters, return values, exceptions

- **Algorithm Comments**: Step-by-step explanation of complex logic

- **Business Logic Comments**: Explain the "why" behind implementation choices

- **Chinese Comments**: Use Chinese for key explanations since teacher is Chinese


### Project Structure

text

LanzhouUni_App/
├── src/
│   ├── core/          # Core business logic
│   ├── ui/           # UI components
│   ├── data/         # Data models
│   └── utils/        # Utility classes
├── resources/        # Resource files (≤300MB)
├── database/         # SQLite database
└── docs/            # Documentation

## Development Guidelines

### Codex Interaction Principles

1. **Modular Development**: Implement one complete small module at a time

2. **Specific Prompts**: Provide clear class structures and functional requirements

3. **Incremental Development**: Start from basic framework, add features progressively

4. **Test-Driven**: Each module should have basic test cases

5. **Compatibility First**: Ensure code runs properly on QT 5.14.2

6. **Detailed Commentary**: Every component must be well-documented for code explanation


### Code Quality Requirements

- ✅ Object-oriented design with proper class hierarchy

- ✅ Template programming (STL containers, generic algorithms)

- ✅ Correct usage of QT signal-slot mechanism

- ✅ Database operations using transactions

- ✅ Safe memory management (RAII principle)

- ✅ Comprehensive error handling

- ✅ DETAILED COMMENTARY FOR ALL COMPONENTS


## Module Development Priority

1. **Basic Framework** (400 lines)

    - Database management

    - User system

    - Main interface framework

2. **Core Systems** (1200 lines)

    - Task system (300 lines)

    - Achievement system (250 lines)

    - Growth system (300 lines)

    - Shop system (250 lines)

    - Log system (200 lines)

3. **UI Optimization** (200 lines)

    - Custom controls

    - Style beautification

    - Interaction optimization


## Pre-configured Settings
-- Built-in test account

Username: you_know_who

Password: assignment
