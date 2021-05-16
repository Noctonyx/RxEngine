
data = {}

data.shaders = {
    ["shader/imgui_vert"] = {
        shader = "/shaders/shdr_imgui_vert.spv",
        stage = "vert"
    },
    ["shader/imgui_frag"] = {
        shader = "/shaders/shdr_imgui_frag.spv",
        stage = "frag"
    },
    ["shader/staticmesh_opaque_vert"] = {
        shader = "/shaders/staticmesh_opaque_vert.spv",
        stage = "vert"
    },
    ["shader/staticmesh_opaque_frag"] = {
        shader = "/shaders/staticmesh_opaque_frag.spv",
        stage = "frag"
    },
}

data.pipeline_layouts = {
    ["layout/imgui"] = {
        ds_layouts = {
            {
                bindings = {
                    {
                        binding = 0,
                        stage = "frag",
                        count = 1,
                        type = "combined-sampler"
                     }                    
                }
            }
        },
        push_constants = {
            {
                stage = "vert",
                offset = 0,
                size = 16
            }
        }
    },
    ["layout/general"] = {
        ds_layouts = {
            {
                bindings = {
                    {
                        binding = 0,
                        stage = "both",
                        count = 1,
                        type = "uniform-buffer-dynamic"
                    },
                    {
                        binding = 1,
                        stage = "both",
                        count = 1,
                        type = "uniform-buffer-dynamic"
                    },
                    {
                        binding = 2,
                        stage = "frag",
                        count = 1,
                        type = "combined-sampler"
                    },
                    {
                        binding = 3,
                        stage = "both",
                        count = 1,
                        type = "storage-buffer"
                    },
                    {
                        binding = 4,
                        stage = "frag",
                        count = 4096,
                        type = "combined-sampler",
                        variable = true,
                        partially_bound = true,
                        update_after = true
                    },
                }
            },
            {
                bindings = {
                    {
                        binding = 0,
                        stage = "vert",
                        count = 1,
                        type = "storage-buffer"
                    },
                }
            },
            {
                bindings = {
                    {
                        binding = 0,
                        stage = "both",
                        count = 1,
                        type = "storage-buffer"
                    },
                }
            }
        },
        push_constants = {
            {
                stage = "vert",
                offset = 0,
                size = 4
            }
        }
    }
};

data.material_pipelines = {
    ["pipeline/imgui"] = {
        layout = "layout/imgui",
        vertexShader = "shader/imgui_vert",
        fragmentShader = "shader/imgui_frag",
        depthTestEnable = false,
        depthWriteEnable = false,
        cullMode = "none",
        blends = {
            {enable = true}
        },
        renderStage = "ui",
        vertices = {
            { type="float", count=2, offset=0 },
            { type="float", count=2, offset=8 },
            { type="byte",  count=4, offset=16},
        }      
    },
    ["pipeline/staticmesh_opaque"] = {
        layout = "layout/general",
        vertexShader = "shader/staticmesh_opaque_vert",
        fragmentShader = "shader/staticmesh_opaque_frag",
        depthTestEnable = true,
        depthWriteEnable = true,
        blends = {
            {enable = false}
        },
        renderStage = "opaque",
        vertices = {
        }      
    },
};

data.textures = {

}

data.materials = {
}

data.meshes = {

}

data.prototypes = {

}

data.visibles = {

}