# VkGameProjectOne

## Compiler shaders

1. Create directories if they don't exist
   ```mkdir -p shaders/compiled```
2. Compile vertex shader
   ```glslc shaders/shader.vert -o shaders/compiled/vert.spv```
3. Compile fragment shader
   ```glslc shaders/shader.frag -o shaders/compiled/frag.spv```