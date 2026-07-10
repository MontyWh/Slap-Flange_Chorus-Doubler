# Copilot Instructions

## Project Guidelines
- For this plugin, prefer auto-typed sample-path locals without Hungarian-style d/f prefixes when adapting audio sample precision.
- Prefer float for internal DSP state but avoid bottlenecking host input/output precision; keep the direct audio I/O path adaptive to the host buffer type.
- Prefer the member name `fSampleRate` rather than `sampleRate` for the processor sample-rate field.
- Apply the f/i naming prefix rule to all float and int variables, including arrays and vectors. Use datatype-based variable naming prefixes in this repo: i for int, f for float, d for double, and s for string (including string arrays/vectors). 
- Remove datatype prefixes from user-defined function parameter names; keep variable prefixes unchanged, and do not alter JUCE-owned APIs.
- Keep all refactor adjustments confined to the project's Source folder and avoid modifying JUCE API/framework-owned code beyond required user-defined implementation details.
- Use the existing code style and formatting, with UK English spelling, when modifying this JUCE plugin codebase.
- The left-hand Rate knobs and the right-hand Time/Div controls serve different purposes and should not be permanently linked to each other.
- Focus on creating a simple tremolo plugin that emphasizes core industry-standard parameters without unnecessary complexity.
- Use industry-standard plugin control terminology for UI labels; avoid nonstandard wording.

## Language Preferences
- Use UK English wording throughout the code and comments.