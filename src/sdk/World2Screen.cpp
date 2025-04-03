#include "World2Screen.hpp"
namespace w2s {
    glm::vec2 WorldToScreen(const glm::vec3& worldPos) {
        SMediator* sMed = devil4_sdk::get_sMediator();
        uCameraCtrl* camera = sMed->camera1;
        glm::mat4 viewMatrix = glm::lookAt(camera->pos, camera->lookat, camera->up);
        glm::vec2 screen = glm::vec2(devil4_sdk::get_sRender()->screenRes);
        float aspectRatio = screen.x / screen.y;
        glm::mat4 projMatrix = glm::perspective(glm::radians(camera->FOV), aspectRatio, 0.1f, 1000.0f);
        glm::vec4 clipPos = projMatrix * viewMatrix * glm::vec4(worldPos, 1.0f);
        if (clipPos.w <= 0.0f) return glm::vec2(-1.0f, -1.0f);
        glm::vec3 ndcPos = glm::vec3(clipPos) / clipPos.w;
        return glm::vec2((ndcPos.x + 1.0f) * 0.5f * screen.x, (1.0f - ndcPos.y) * 0.5f * screen.y);
    }

    float GetDistanceFromCam(const glm::vec3& targetPos) {
        SMediator* sMed = devil4_sdk::get_sMediator();
        uCameraCtrl* camera = sMed->camera1;
        glm::vec3 cameraPos = camera->pos;
        return glm::distance(cameraPos, targetPos);
    }

    void DrawWireframeCube(const glm::vec3& center, float size, float rotation, ImU32 color, float thickness) {
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        glm::vec3 vertices[8];
        float halfSize = size / 2.0f;

        vertices[0] = glm::vec3(-halfSize, -halfSize, -halfSize);
        vertices[1] = glm::vec3(halfSize, -halfSize, -halfSize);
        vertices[2] = glm::vec3(halfSize, halfSize, -halfSize);
        vertices[3] = glm::vec3(-halfSize, halfSize, -halfSize);
        vertices[4] = glm::vec3(-halfSize, -halfSize, halfSize);
        vertices[5] = glm::vec3(halfSize, -halfSize, halfSize);
        vertices[6] = glm::vec3(halfSize, halfSize, halfSize);
        vertices[7] = glm::vec3(-halfSize, halfSize, halfSize);

        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), rotation, glm::vec3(0.0f, 1.0f, 0.0f));

        glm::vec3 transformedVertices[8];
        for (int i = 0; i < 8; i++) {
            glm::vec4 rotated = rotationMatrix * glm::vec4(vertices[i], 1.0f);
            transformedVertices[i] = glm::vec3(rotated);

            transformedVertices[i] += center;
        }

        ImVec2 screenVertices[8];
        for (int i = 0; i < 8; i++) {
            glm::vec2 screen = w2s::WorldToScreen(transformedVertices[i]);
            screenVertices[i] = ImVec2(screen.x, screen.y);
        }

        // Bottom face
        drawList->AddLine(screenVertices[0], screenVertices[1], color, thickness);
        drawList->AddLine(screenVertices[1], screenVertices[2], color, thickness);
        drawList->AddLine(screenVertices[2], screenVertices[3], color, thickness);
        drawList->AddLine(screenVertices[3], screenVertices[0], color, thickness);

        // Top face
        drawList->AddLine(screenVertices[4], screenVertices[5], color, thickness);
        drawList->AddLine(screenVertices[5], screenVertices[6], color, thickness);
        drawList->AddLine(screenVertices[6], screenVertices[7], color, thickness);
        drawList->AddLine(screenVertices[7], screenVertices[4], color, thickness);

        // Connecting edges
        drawList->AddLine(screenVertices[0], screenVertices[4], color, thickness);
        drawList->AddLine(screenVertices[1], screenVertices[5], color, thickness);
        drawList->AddLine(screenVertices[2], screenVertices[6], color, thickness);
        drawList->AddLine(screenVertices[3], screenVertices[7], color, thickness);
    }

    void DrawWireframeSphere(const glm::vec3& center, float radius, float rotation, ImU32 color, int segments, float thickness) {
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();

        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), rotation, glm::vec3(0.0f, 1.0f, 0.0f));

        // Create three circles to represent the sphere (XY, XZ, YZ planes)
        for (int i = 0; i < segments; i++) {
            float angle = 2.0f * glm::pi<float>() * i / segments;
            float nextAngle = 2.0f * glm::pi<float>() * (i + 1) / segments;

            // XY plane circle
            glm::vec3 pointXY1(radius * cos(angle), radius * sin(angle), 0);
            glm::vec3 pointXY2(radius * cos(nextAngle), radius * sin(nextAngle), 0);

            // XZ plane circle
            glm::vec3 pointXZ1(radius * cos(angle), 0, radius * sin(angle));
            glm::vec3 pointXZ2(radius * cos(nextAngle), 0, radius * sin(nextAngle));

            // YZ plane circle
            glm::vec3 pointYZ1(0, radius * cos(angle), radius * sin(angle));
            glm::vec3 pointYZ2(0, radius * cos(nextAngle), radius * sin(nextAngle));

            // Transform points with rotation
            glm::vec3 rotatedXY1 = glm::vec3(rotationMatrix * glm::vec4(pointXY1, 1.0f));
            glm::vec3 rotatedXY2 = glm::vec3(rotationMatrix * glm::vec4(pointXY2, 1.0f));
            glm::vec3 rotatedXZ1 = glm::vec3(rotationMatrix * glm::vec4(pointXZ1, 1.0f));
            glm::vec3 rotatedXZ2 = glm::vec3(rotationMatrix * glm::vec4(pointXZ2, 1.0f));
            glm::vec3 rotatedYZ1 = glm::vec3(rotationMatrix * glm::vec4(pointYZ1, 1.0f));
            glm::vec3 rotatedYZ2 = glm::vec3(rotationMatrix * glm::vec4(pointYZ2, 1.0f));

            // Translate to center
            rotatedXY1 += center;
            rotatedXY2 += center;
            rotatedXZ1 += center;
            rotatedXZ2 += center;
            rotatedYZ1 += center;
            rotatedYZ2 += center;

            // Convert to screen space
            glm::vec2 screenXY1 = w2s::WorldToScreen(rotatedXY1);
            glm::vec2 screenXY2 = w2s::WorldToScreen(rotatedXY2);
            glm::vec2 screenXZ1 = w2s::WorldToScreen(rotatedXZ1);
            glm::vec2 screenXZ2 = w2s::WorldToScreen(rotatedXZ2);
            glm::vec2 screenYZ1 = w2s::WorldToScreen(rotatedYZ1);
            glm::vec2 screenYZ2 = w2s::WorldToScreen(rotatedYZ2);

            // Draw the lines
            drawList->AddLine(ImVec2(screenXY1.x, screenXY1.y), ImVec2(screenXY2.x, screenXY2.y), color, thickness);
            drawList->AddLine(ImVec2(screenXZ1.x, screenXZ1.y), ImVec2(screenXZ2.x, screenXZ2.y), color, thickness);
            drawList->AddLine(ImVec2(screenYZ1.x, screenYZ1.y), ImVec2(screenYZ2.x, screenYZ2.y), color, thickness);
        }
    }

    void DrawLine3D(const glm::vec3& start, const glm::vec3& end, ImU32 color, float thickness) {
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();

        // Convert to screen space
        glm::vec2 screenStart = w2s::WorldToScreen(start);
        glm::vec2 screenEnd = w2s::WorldToScreen(end);

        // Draw the line
        drawList->AddLine(ImVec2(screenStart.x, screenStart.y), ImVec2(screenEnd.x, screenEnd.y), color, thickness);
    }

    glm::mat4 CreateRotationMatrix(float pitch, float yaw, float roll) {
        glm::mat4 rotMatrix = glm::mat4(1.0f);
        rotMatrix = glm::rotate(rotMatrix, pitch, glm::vec3(1.0f, 0.0f, 0.0f));  // X-axis
        rotMatrix = glm::rotate(rotMatrix, yaw, glm::vec3(0.0f, 1.0f, 0.0f));    // Y-axis
        rotMatrix = glm::rotate(rotMatrix, roll, glm::vec3(0.0f, 0.0f, 1.0f));   // Z-axis

        return rotMatrix;
    }

    void DrawWireframeTriangle(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, ImU32 color, float thickness) {
        // First check if any of the points are visible
        if (!IsVisibleOnScreen(v1) && !IsVisibleOnScreen(v2) && !IsVisibleOnScreen(v3)) {
            return; // Skip rendering if all points are off-screen
        }

        ImDrawList* drawList = ImGui::GetBackgroundDrawList();

        // Convert vertices to screen space
        glm::vec2 screenV1 = w2s::WorldToScreen(v1);
        glm::vec2 screenV2 = w2s::WorldToScreen(v2);
        glm::vec2 screenV3 = w2s::WorldToScreen(v3);

        // Check if any vertex is behind the camera (indicated by -1,-1)
        if ((screenV1.x == -1.0f && screenV1.y == -1.0f) ||
            (screenV2.x == -1.0f && screenV2.y == -1.0f) ||
            (screenV3.x == -1.0f && screenV3.y == -1.0f)) {
            return; // Skip if any vertex is behind camera
        }

        // Draw the three lines that make up the triangle
        drawList->AddLine(
            ImVec2(screenV1.x, screenV1.y),
            ImVec2(screenV2.x, screenV2.y),
            color, thickness
        );

        drawList->AddLine(
            ImVec2(screenV2.x, screenV2.y),
            ImVec2(screenV3.x, screenV3.y),
            color, thickness
        );

        drawList->AddLine(
            ImVec2(screenV3.x, screenV3.y),
            ImVec2(screenV1.x, screenV1.y),
            color, thickness
        );
    }

    void DrawFilledTriangle(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, ImU32 color) {
        // First check if any of the points are visible
        if (!IsVisibleOnScreen(v1) && !IsVisibleOnScreen(v2) && !IsVisibleOnScreen(v3)) {
            return; // Skip rendering if all points are off-screen
        }

        ImDrawList* drawList = ImGui::GetBackgroundDrawList();

        // Convert vertices to screen space
        glm::vec2 screenV1 = w2s::WorldToScreen(v1);
        glm::vec2 screenV2 = w2s::WorldToScreen(v2);
        glm::vec2 screenV3 = w2s::WorldToScreen(v3);

        // Check if any vertex is behind the camera
        if ((screenV1.x == -1.0f && screenV1.y == -1.0f) ||
            (screenV2.x == -1.0f && screenV2.y == -1.0f) ||
            (screenV3.x == -1.0f && screenV3.y == -1.0f)) {
            return; // Skip if any vertex is behind camera
        }

        // Draw the filled triangle
        drawList->AddTriangleFilled(
            ImVec2(screenV1.x, screenV1.y),
            ImVec2(screenV2.x, screenV2.y),
            ImVec2(screenV3.x, screenV3.y),
            color
        );
    }

    // Helper function to create regular polygons or complex shapes made of triangles
    void DrawWireframeMesh(const std::vector<glm::vec3>& vertices, const std::vector<std::vector<int>>& triangles, ImU32 color, float thickness) {
        // Check if any vertices are visible before processing
        bool anyVisible = false;
        for (const auto& vertex : vertices) {
            if (IsVisibleOnScreen(vertex)) {
                anyVisible = true;
                break;
            }
        }

        if (!anyVisible) return;

        // Draw each triangle in the mesh
        for (const auto& tri : triangles) {
            // Make sure the triangle has at least 3 indices
            if (tri.size() >= 3) {
                DrawWireframeTriangle(
                    vertices[tri[0]],
                    vertices[tri[1]],
                    vertices[tri[2]],
                    color, thickness
                );
            }
        }
    }

    // Alternative version using array of indices (3 per triangle)
    void DrawWireframeMeshIndices(const std::vector<glm::vec3>& vertices, const std::vector<int>& indices, ImU32 color, float thickness) {
        // Check if any vertices are visible before processing
        bool anyVisible = false;
        for (const auto& vertex : vertices) {
            if (IsVisibleOnScreen(vertex)) {
                anyVisible = true;
                break;
            }
        }

        if (!anyVisible) return;

        // Draw each triangle in the mesh
        // Indices should come in groups of 3 (one triangle)
        for (size_t i = 0; i < indices.size() - 2; i += 3) {
            DrawWireframeTriangle(
                vertices[indices[i]],
                vertices[indices[i + 1]],
                vertices[indices[i + 2]],
                color, thickness
            );
        }
    }

    bool IsVisibleOnScreen(const glm::vec3& worldPos, float objectRadius) {
        // Convert world position to screen space
        glm::vec2 screenPos = WorldToScreen(worldPos);

        // Get screen dimensions
        glm::vec2 screen = glm::vec2(devil4_sdk::get_sRender()->screenRes);

        // Simple check if the position is outside the screen borders
        // The -1.0f return value from WorldToScreen indicates the point is behind the camera
        if (screenPos.x == -1.0f && screenPos.y == -1.0f) {
            return false;
        }

        // For objects with size, we add a buffer equal to their radius in screen space
        float screenBuffer = 0.0f;
        if (objectRadius > 0.0f) {
            float distFromCamera = GetDistanceFromCam(worldPos);
            // Simple approximation of screen radius
            screenBuffer = objectRadius * 100.0f / distFromCamera;
        }

        // Check if the object is within the screen bounds, with buffer
        if (screenPos.x + screenBuffer < 0 || screenPos.x - screenBuffer > screen.x ||
            screenPos.y + screenBuffer < 0 || screenPos.y - screenBuffer > screen.y) {
            return false;
        }

        return true;
    }

    /*void ScreenToRay(const glm::vec2& screenPos, glm::vec3& rayOrigin, glm::vec3& rayDir) {
        SMediator* sMed = devil4_sdk::get_sMediator();
        uCameraCtrl* camera = sMed->camera1;

        // Get screen dimensions
        glm::vec2 screen = glm::vec2(devil4_sdk::get_sRender()->screenRes);
        float aspectRatio = screen.x / screen.y;

        // Convert screen pos to normalized device coordinates [-1, 1]
        glm::vec2 ndcPos;
        ndcPos.x = (2.0f * screenPos.x / screen.x) - 1.0f;
        ndcPos.y = 1.0f - (2.0f * screenPos.y / screen.y) * 2.0f;

        // Create inverse view-projection matrix
        glm::mat4 viewMatrix = glm::lookAt(camera->pos, camera->lookat, camera->up);
        glm::mat4 projMatrix = glm::perspective(glm::radians(camera->FOV), aspectRatio, 0.1f, 1000.0f);
        glm::mat4 invViewProj = glm::inverse(projMatrix * viewMatrix);

        // Calculate ray
        glm::vec4 rayStart = invViewProj * glm::vec4(ndcPos.x, ndcPos.y, -1.0f, 1.0f);
        glm::vec4 rayEnd = invViewProj * glm::vec4(ndcPos.x, ndcPos.y, 1.0f, 1.0f);

        rayStart /= rayStart.w;
        rayEnd /= rayEnd.w;

        rayOrigin = camera->pos;
        rayDir = glm::normalize(glm::vec3(rayEnd) - glm::vec3(rayStart));
    }

    bool RayPlaneIntersection(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& planePos, const glm::vec3& planeNormal, float& t) {
        float denom = glm::dot(planeNormal, rayDir);
        if (abs(denom) < 0.0001f) {
            return false; // Ray is parallel to plane
        }

        t = glm::dot(planePos - rayOrigin, planeNormal) / denom;
        return t > 0; // Only positive t (in front of camera)
    }*/

    void DrawTextureWithScreenCorners(LPDIRECT3DDEVICE9 device, LPDIRECT3DTEXTURE9 texture, const glm::vec2 corners[4]) {
        if (!device || !texture)
            return;

        // Create vertices for a quad
        TexturedVertex vertices[4];

        // Get viewport info for conversion to NDC
        D3DVIEWPORT9 viewport;
        device->GetViewport(&viewport);
        float screenWidth = static_cast<float>(viewport.Width);
        float screenHeight = static_cast<float>(viewport.Height);

        // Convert screen coordinates to normalized device coordinates (-1 to 1)
        for (int i = 0; i < 4; i++) {
            float x = corners[i].x;
            float y = corners[i].y;
            float ndcX = (x / screenWidth) * 2.0f - 1.0f;
            float ndcY = -((y / screenHeight) * 2.0f - 1.0f);  // Y is inverted

            vertices[i].position = D3DXVECTOR3(ndcX, ndcY, 0.0f);
        }

        // Texture coordinates
        vertices[0].texcoord = D3DXVECTOR2(0.0f, 0.0f);  // Top-left
        vertices[1].texcoord = D3DXVECTOR2(1.0f, 0.0f);  // Top-right
        vertices[2].texcoord = D3DXVECTOR2(0.0f, 1.0f);  // Bottom-left
        vertices[3].texcoord = D3DXVECTOR2(1.0f, 1.0f);  // Bottom-right

        // Store original state
        DWORD oldLighting, oldZEnable, oldCullMode;
        device->GetRenderState(D3DRS_LIGHTING, &oldLighting);
        device->GetRenderState(D3DRS_ZENABLE, &oldZEnable);
        device->GetRenderState(D3DRS_CULLMODE, &oldCullMode);

        // Store original matrices
        D3DXMATRIX oldWorld, oldView, oldProj;
        device->GetTransform(D3DTS_WORLD, &oldWorld);
        device->GetTransform(D3DTS_VIEW, &oldView);
        device->GetTransform(D3DTS_PROJECTION, &oldProj);

        // Set identity matrices for rendering in screen space
        D3DXMATRIX identity;
        D3DXMatrixIdentity(&identity);
        device->SetTransform(D3DTS_WORLD, &identity);
        device->SetTransform(D3DTS_VIEW, &identity);
        device->SetTransform(D3DTS_PROJECTION, &identity);

        // Set our rendering states for 2D rendering
        device->SetRenderState(D3DRS_LIGHTING, FALSE);
        device->SetRenderState(D3DRS_ZENABLE, FALSE);  // Disable Z-buffer for 2D overlay
        device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

        // Enable alpha blending
        DWORD oldAlphaBlend, oldSrcBlend, oldDestBlend;
        device->GetRenderState(D3DRS_ALPHABLENDENABLE, &oldAlphaBlend);
        device->GetRenderState(D3DRS_SRCBLEND, &oldSrcBlend);
        device->GetRenderState(D3DRS_DESTBLEND, &oldDestBlend);
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

        // Set texture
        device->SetTexture(0, texture);
        device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
        device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

        // Use triangle strip to draw the quad
        device->SetFVF(TexturedVertex::FVF);
        device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(TexturedVertex));

        // Restore previous state
        device->SetTexture(0, NULL);
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, oldAlphaBlend);
        device->SetRenderState(D3DRS_SRCBLEND, oldSrcBlend);
        device->SetRenderState(D3DRS_DESTBLEND, oldDestBlend);
        device->SetRenderState(D3DRS_LIGHTING, oldLighting);
        device->SetRenderState(D3DRS_ZENABLE, oldZEnable);
        device->SetRenderState(D3DRS_CULLMODE, oldCullMode);

        // Restore original matrices
        device->SetTransform(D3DTS_WORLD, &oldWorld);
        device->SetTransform(D3DTS_VIEW, &oldView);
        device->SetTransform(D3DTS_PROJECTION, &oldProj);
    }

    void DrawTextureWithPerspective(LPDIRECT3DDEVICE9 g_pd3dDevice, PDIRECT3DTEXTURE9 m_texture_handle, const glm::vec3& worldPos, float width, float height, const glm::mat4& viewMatrix, float yRotationDegrees) {
        glm::vec3 localCorners[4];
        float halfWidth = width / 2.0f;
        float halfHeight = height / 2.0f;

        // Define corners in local space (XY plane)
        localCorners[0] = glm::vec3(-halfWidth, halfHeight, 0.0f);  // Top-left
        localCorners[1] = glm::vec3(halfWidth, halfHeight, 0.0f);   // Top-right
        localCorners[2] = glm::vec3(-halfWidth, -halfHeight, 0.0f); // Bottom-left
        localCorners[3] = glm::vec3(halfWidth, -halfHeight, 0.0f);  // Bottom-right

        float yRotationRadians = glm::radians(yRotationDegrees);
        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), yRotationRadians, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::vec3 worldCorners[4];
        for (int i = 0; i < 4; i++) {
            glm::vec4 rotatedCorner = rotationMatrix * glm::vec4(localCorners[i], 1.0f);
            worldCorners[i] = worldPos + glm::vec3(rotatedCorner);
        }

        // Project all corners to screen space
        glm::vec2 screenCorners[4];
        for (int i = 0; i < 4; i++) {
            screenCorners[i] = w2s::WorldToScreen(worldCorners[i]);
        }
        DrawTextureWithScreenCorners(g_pd3dDevice, m_texture_handle, screenCorners);
    }
}