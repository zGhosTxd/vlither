#include "bp_renderer.h"
#include <stdlib.h>
#include <string.h>
#include <graphics/ig_buffer.h>

bp_renderer* bp_renderer_create(ig_context* context, unsigned int max_instances) {
    bp_renderer* r = malloc(sizeof(bp_renderer));
    r->instance_count = 0;
    r->instances = malloc(max_instances * sizeof(bp_instance));

    VkShaderModule vertex_shader = ig_context_create_shader_from_file(context, "app/res/shaders/bpv.spv");
    VkShaderModule fragment_shader = ig_context_create_shader_from_file(context, "app/res/shaders/bpf.spv");

    vkCreateGraphicsPipelines(context->device, VK_NULL_HANDLE, 1, &(VkGraphicsPipelineCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = (VkPipelineShaderStageCreateInfo[]) {
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = vertex_shader,
                .pName = "main",
            },
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = fragment_shader,
                .pName = "main",
            }
        },
        .pVertexInputState = &(VkPipelineVertexInputStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 2,
            .pVertexBindingDescriptions = (VkVertexInputBindingDescription[]) {
                {   // posição UV (mesh base do segmento)
                    .binding = 0,
                    .stride = 2 * sizeof(float),
                    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
                },
                {   // instancia
                    .binding = 1,
                    .stride = sizeof(bp_instance),
                    .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
                },
            },

            // ⬇️ 8 atributos agora
            .vertexAttributeDescriptionCount = 8,
            .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription[]) {

                // posição mesh
                { .location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = 0 },

                // circ
                { .location = 1, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(bp_instance, circ) },

                // ratios
                { .location = 2, .binding = 1, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(bp_instance, ratios) },

                // color
                { .location = 3, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(bp_instance, color) },

                // shadow
                { .location = 4, .binding = 1, .format = VK_FORMAT_R32_SFLOAT, .offset = offsetof(bp_instance, shadow) },

                // eye
                { .location = 5, .binding = 1, .format = VK_FORMAT_R32_SFLOAT, .offset = offsetof(bp_instance, eye) },

                // texpos (PARA LINHA BRANCA)
                { .location = 6, .binding = 1, .format = VK_FORMAT_R32_SFLOAT, .offset = offsetof(bp_instance, texpos) },

                // is_local (1 = minha cobra)
                { .location = 7, .binding = 1, .format = VK_FORMAT_R32_SFLOAT, .offset = offsetof(bp_instance, is_local) }
            }
        },

        .pInputAssemblyState = &(VkPipelineInputAssemblyStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        },

        .pViewportState = &(VkPipelineViewportStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount = 1
        },

        .pRasterizationState = &(VkPipelineRasterizationStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        },

        .pMultisampleState = &(VkPipelineMultisampleStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        },

        .pColorBlendState = &(VkPipelineColorBlendStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &(VkPipelineColorBlendAttachmentState) {
                .blendEnable = VK_TRUE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask = 0xF
            }
        },

        .pDynamicState = &(VkPipelineDynamicStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = 2,
            .pDynamicStates = (VkDynamicState[]) {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
            }
        },

        .layout = context->standard_layout,
        .renderPass = context->default_frame.render_pass,
        .subpass = 0

    }, NULL, &r->pipeline);

    vkDestroyShaderModule(context->device, fragment_shader, NULL);
    vkDestroyShaderModule(context->device, vertex_shader, NULL);

    r->instance_buffer = ig_context_dbuffer_create(context, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, NULL, max_instances * sizeof(bp_instance));

    return r;
}

void bp_renderer_push(bp_renderer* renderer, const bp_instance* inst) {
    renderer->instances[renderer->instance_count++] = *inst;
}

void bp_renderer_flush(bp_renderer* r, ig_context* context, _ig_frame* frame) {
    memcpy(
        r->instance_buffer[context->frame_idx].data,
        r->instances,
        r->instance_count * sizeof(bp_instance)
    );

    vkCmdBindPipeline(frame->cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, r->pipeline);

    vkCmdBindVertexBuffers(frame->cmd_buffer, 1, 1,
        &r->instance_buffer[context->frame_idx].buffer,
        (VkDeviceSize[]) { 0 });

    vkCmdDraw(frame->cmd_buffer, 4, r->instance_count, 0, 0);

    r->instance_count = 0;
}

void bp_renderer_destroy(bp_renderer* r, ig_context* context) {
    ig_context_dbuffer_destroy(context, r->instance_buffer);
    vkDestroyPipeline(context->device, r->pipeline, NULL);
    free(r->instances);
    free(r);
}
