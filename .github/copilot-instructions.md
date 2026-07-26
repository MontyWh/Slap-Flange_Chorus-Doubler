# Copilot Instructions

## Project Guidelines
- For this plugin, prefer auto-typed sample-path locals without Hungarian-style d/f prefixes when adapting audio sample precision.
- Prefer float for internal DSP state and processing; avoid bottlenecking host input/output precision; keep the direct audio I/O path adaptive to the host buffer type.
- Prefer the member name `fSampleRate` rather than `sampleRate` for the processor sample-rate field.
- Apply the f/i naming prefix rule to all float and int variables, including arrays and vectors. Use datatype-based variable naming prefixes in this repo: i for int, f for float, d for double, and s for string (including string arrays/vectors). 
- Remove datatype prefixes from user-defined function parameter names; keep variable prefixes unchanged, and do not alter JUCE-owned APIs.
- Keep all refactor adjustments confined to the project's Source folder and avoid modifying JUCE API/framework-owned code beyond required user-defined implementation details.
- Use the existing code style and formatting, with UK English spelling, when modifying this JUCE plugin codebase.
- The left-hand Rate knobs and the right-hand Time/Div controls serve different purposes and should not be permanently linked to each other.
- Focus on creating a simple tremolo plugin that emphasizes core industry-standard parameters without unnecessary complexity.
- Use industry-standard plugin control terminology for UI labels; avoid nonstandard wording.
- Apply edits only to existing Projucer-managed files; do not create temporary/generated side files in the Source folder.
- Project author attribution to use in file banners: Plugin: AutoTremolando, GitHub: MontyWh, Author: Montague Whishaw.
- Prefer Projucer-generated template files to remain mostly intact (comments and standard methods), with logic added by filling existing functions where possible rather than introducing many new methods.
- Remove disused helper functions after merging into the Projucer float processBlock template style.

## Logic Placement
- When moving logic to PluginExtra.h, only move DSP math/processing helpers there; keep UI-related logic and metadata out of PluginExtra.
- Encapsulate smoothing logic in Source/PluginExtra.h and invoke it from other files while preserving existing behaviour and code format.
- Use grouped section header comments above logical code groups (not function/class-level header comments), while keeping key right-side inline comments for important DSP lines.
- Comment headers must use a three-line separator format (//======================================================================, // <section title>, //======================================================================), and all comments must use UK spelling.

## Language Preferences
- Use UK English wording throughout the code and comments.
- Prefer inline end-of-line comments instead of separate comment lines when requesting code annotation.
- Use meaningful comments only; remove generic filler comments. Add meaningful, non-filler comments across all project source files when requesting documentation updates.