
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

data.material_pipelines = {
    ["pipeline.imgui"] = {
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
