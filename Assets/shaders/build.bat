glslc --target-env=vulkan1.2  -o shdr_imgui_vert.spv shdr_imgui.vert
glslc --target-env=vulkan1.2  -o shdr_imgui_frag.spv shdr_imgui.frag
glslc --target-env=vulkan1.2  -o rml_frag.spv rml.frag
glslc --target-env=vulkan1.2  -o rml_vert.spv rml.vert
glslc --target-env=vulkan1.2  -o staticmesh_opaque_vert.spv staticmesh_opaque.vert
glslc --target-env=vulkan1.2  -o staticmesh_opaque_frag.spv staticmesh_opaque.frag
glslc --target-env=vulkan1.2  -o screenquad_vert.spv screenquad.vert

