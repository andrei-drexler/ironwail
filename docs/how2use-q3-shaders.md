# Working with Quake III Shader Files in Ironwail

Ironwail now ships with a lightweight Quake III shader parser. This document explains how to load shader definition files at runtime, inspect the parsed data, and use the new C API from engine code.

## Console workflow

1. **Load shader files**

   ```
   q3shader_load scripts/common.shader
   ```

   * The command accepts one or more relative paths (use forward slashes).
   * Each invocation reports how many shader blocks were successfully parsed.

   To pull in *every* `scripts/*.shader` file that is visible through the Quake filesystem, run:

   ```
   q3shader_loadall
   ```

   * Searches PAKs and loose directories in priority order (mods override base content).
   * Prints a summary of how many files succeeded versus failed and the total shader count discovered.

2. **List the currently loaded shaders**

   ```
   q3shader_list
   ```

   * Displays every shader name and the number of stages the parser discovered for that entry.

3. **Inspect a single shader**

   ```
   q3shader_info textures/common/nodraw
   ```

   The command prints:

   * Source file the definition came from.
   * Cull mode and decoded `surfaceparm` flags.
   * Metadata such as `qer_editorimage`, transparency, and light parameters when present.
   * Stage-by-stage details (maps, animMap rate, blend/alpha configuration, tcMods, etc.).

4. **Clear all parsed shader data**

   ```
   q3shader_clear
   ```

   * Useful when reloading a modified `.shader` file while developing content.

## C API overview

Include `q3shader.h` to access the parser programmatically. Key helpers:

* `int Q3Shader_LoadFile(const char *path);`
  * Parses a `.shader` file and merges/replaces entries in the registry.
* `int Q3Shader_LoadAll(int *files_loaded, int *files_failed);`
  * Enumerates `scripts/*.shader` across the virtual filesystem.
  * Returns the number of shader definitions parsed and optionally reports the number of files that succeeded/failed.
* `void Q3Shader_Clear(void);`
  * Empties the registry.
* `size_t Q3Shader_Count(void);`
  * Returns the number of loaded shaders. Iterate with `Q3Shader_GetByIndex`.
* `const q3shader_t *Q3Shader_Find(const char *name);`
  * Fetches a shader definition by name (case-insensitive).
* `void Q3Shader_DescribeSurfaceParms(const q3shader_t *shader, char *buffer, size_t bufsize);`
  * Converts the surface parameter bit-mask into a readable string.
* Stage helpers such as `Q3ShaderStage_Count`, `Q3ShaderStage_GetMap`, `Q3ShaderStage_GetAnimMapCount`, and `Q3ShaderStage_GetTcModCount` expose render-stage data.

### Sample iteration code

```c
for (size_t i = 0; i < Q3Shader_Count(); ++i) {
    const q3shader_t *shader = Q3Shader_GetByIndex(i);
    Con_Printf("%s has %zu stage(s)\n", shader->name, Q3ShaderStage_Count(shader));

    for (size_t s = 0; s < Q3ShaderStage_Count(shader); ++s) {
        const q3shader_stage_t *stage = Q3Shader_GetStage(shader, s);
        const char *map = Q3ShaderStage_GetMap(stage);
        if (map)
            Con_Printf("  stage %zu map: %s\n", s + 1, map);
    }
}
```

## Notes and limitations

* The parser is designed for Quake III style syntax, including nested stage blocks, `surfaceparm` directives, `animMap`, `tcMod`, and blend instructions. Unknown directives are still stored in the directive lists so nothing is lost.
* Definitions loaded later replace earlier entries that share the same shader name, mirroring idTech 3 behaviour.
* Shader data lives in zone memory and is released automatically during `q3shader_clear` or engine shutdown.
