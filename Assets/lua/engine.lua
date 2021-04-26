
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
};

data.textures = {

}

data.materials = {
}

data.meshes = {

}