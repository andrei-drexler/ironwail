# Cascaded Shadow Maps

Ironwail now supports cascaded shadow maps (CSM) for large outdoor scenes. The
renderer builds up to four shadow cascades per frame, blends between them in
the forward pass, and exposes debug tooling and presets to help tune quality
versus performance.

## Enabling shadows

1. Enable shadow mapping via `r_shadows 1`.
2. Enable cascades with `r_shadow_csm 1` (the default). Setting the CVar to `0`
   falls back to a single shadow layer.
3. Adjust the cascade count with `r_shadow_csm_splits` (valid range: `2` to `4`
   splits when cascades are enabled). The engine automatically reconfigures the
   shadow texture array when the value changes.
4. Optional: toggle stable cascades with `r_shadow_csm_stable`. Stable cascades
   snap the light-space projection to texel-sized increments to minimize
   shimmering when the camera moves.

All cascades are stored in a `GL_TEXTURE_2D_ARRAY`. The renderer handles
resource (re-)allocation automatically when any of the cascade-related CVars
change.

## Core CVars

| CVar | Description |
| --- | --- |
| `r_shadow_map_size` | Side length (in texels) of each cascade. Values are clamped to 1024, 2048, or 4096. |
| `r_shadow_bias` / `r_shadow_slope_bias` | Constant and slope polygon offset used while rasterizing the depth maps. |
| `r_shadow_soft` / `r_shadow_pcf_size` | Enables simple PCF filtering and scales the sample radius (1–3). |
| `r_shadow_normal_offset` | Receiver-plane depth offset used to reduce peter-panning. |
| `r_shadow_vsm` / `r_shadow_vsm_bleed_reduce` | Enables variance shadow maps and controls light-bleeding reduction. |
| `r_shadow_csm` | Enables cascaded shadow maps (1) or single-layer shadows (0). |
| `r_shadow_csm_splits` | Number of cascades to render (minimum 2, maximum 4 when CSM is enabled). |
| `r_shadow_csm_stable` | Snaps cascades to light-space texel centers each frame to avoid shimmering. |
| `r_shadow_csm_fade` | Width of the blend region between adjacent cascades (0 disables morphing). |
| `r_shadow_quality` | Quality preset that adjusts `r_shadow_map_size`, `r_shadow_csm_splits`, and `r_shadow_soft`. |
| `r_shadow_showcsm` | When set to 1, tints pixels by the cascade that contributed the shadow result. |
| `r_shadow_showmap` | Displays the contents of a specific cascade layer (1-based index). Set to 0 to disable. |

### Quality presets

`r_shadow_quality` provides three curated presets:

| Value | Map size | Cascades | Filtering |
| --- | --- | --- | --- |
| `0` (Low) | 1024 | 2 | Hard shadows |
| `1` (Medium) | 1536 | 3 | PCF enabled |
| `2` (High) | 2048 | 4 | PCF enabled |

You can still override the underlying CVars after selecting a preset.

## Debug overlays

* `r_shadow_showcsm 1` renders a semi-transparent cascade ID overlay so you can
  verify split transitions and coverage.
* `r_shadow_showmap <n>` displays the depth contents of cascade `n` in the
  corner of the screen. Use this to inspect coverage, resolution, and bias
  issues. Set the value back to `0` to hide the debug view.

Both overlays can be used simultaneously. The forward shader also blends
between adjacent cascades using a configurable fade distance to minimize seams
at split boundaries.

## Performance notes

* The engine performs both camera-frustum and cascade-frustum culling during
  the shadow pass. Entities outside the active cascade or outside the camera
  frustum are skipped, reducing draw calls.
* Brush models are batched by material as before; translucent brush models are
  still processed separately to preserve correct state ordering.
* Shadow maps are rendered cascade-by-cascade by rebinding the depth texture
  array layer on the fly, so no additional FBOs are required.

## Troubleshooting

* **Shimmering or flickering shadows** &mdash; Enable `r_shadow_csm_stable 1` to
  quantize cascades to texels, and adjust `r_shadow_bias` /
  `r_shadow_slope_bias` if acne persists.
* **Missing shadows in the distance** &mdash; Increase `r_shadow_csm_splits` or
  `r_shadow_map_size`, or raise `r_shadow_quality` for higher preset values.
* **Artifacts at cascade boundaries** &mdash; Ensure `r_shadow_soft 1` with a
  moderate PCF radius, keep `r_shadow_csm_splits` at 3–4 for wide outdoor
  views, and tune `r_shadow_csm_fade` to adjust the blend width between
  cascades.
* **Performance drops** &mdash; Reduce cascade count, lower `r_shadow_map_size`, or
  switch to the Low preset via `r_shadow_quality 0`.

Set `r_shadow_csm 0` or `r_shadows 0` for a legacy single-layer shadow fallback
if needed.
