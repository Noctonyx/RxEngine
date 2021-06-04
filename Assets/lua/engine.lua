serpent = require('util/serpent')
data = require('util/data')

data:extend(
  {
    {
      type = 'shader',
      name = "shader/imgui_vert",
      shader = "/shaders/shdr_imgui_vert.spv",
      stage = "vert"
    },
    {
      type = 'shader',
      name = "shader/imgui_frag",
      shader = "/shaders/shdr_imgui_frag.spv",
      stage = "frag"
    },
    {
      type = 'shader',
      name = "shader/rmlui_vert",
      shader = "/shaders/rml_vert.spv",
      stage = "vert"
    },
    {
      type = 'shader',
      name = "shader/rmlui_frag",
      shader = "/shaders/rml_frag.spv",
      stage = "frag"
    },
    {
      type = 'shader',
      name = "shader/staticmesh_opaque_vert",
      shader = "/shaders/staticmesh_opaque_vert.spv",
      stage = "vert"
    },
    {
      type = 'shader',
      name = "shader/staticmesh_opaque_frag",
      shader = "/shaders/staticmesh_opaque_frag.spv",
      stage = "frag"
    }
  }
)

data:extend(
  {
    {
      type = "pipeline_layout",
      name = "layout/imgui",
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
    {
      type = "pipeline_layout",
      name = "layout/rmlui",
      ds_layouts = {
        {
          bindings = {
            {
              binding = 0,
              type = "uniform-buffer",
              stage = "vert",
              count = 1
            },
            {
              binding = 1,
              stage = "frag",
              count = 4096,
              type = "combined-sampler",
              variable = true,
              partially_bound = true,
              update_after = true
            },
          }
        }
      },
      push_constants = {
        {
          stage = "both",
          offset = 0,
          size = 144
        }
      }
    },
    {
        type = "pipeline_layout",
        name = "layout/general",
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
                size = 16
            }
        }
    }
  }
)

data:extend(
  {
    {
        type = "material_pipeline",
        name = "pipeline/imgui",
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
    {
        type = "material_pipeline",
        name = "pipeline/staticmesh_opaque",
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
    {
        type = "material_pipeline",
        name = "pipeline/rmlui",
        layout = "layout/rmlui",
        vertexShader = "shader/rmlui_vert",
        fragmentShader = "shader/rmlui_frag",
        depthTestEnable = false,
        depthWriteEnable = false,
        cullMode = "none",
        blends = {
            {enable = true}
        },
        renderStage = "ui",
        vertices = {
            { type="float",  count=2, offset=0  },
            { type="byte",   count=4, offset=8  },
            { type="float",  count=2, offset=12 },
        }
    }
  }
)