#include "World2Screen.hpp"
#include <d3d9.h>
#define DEBUG_DRAW_IMPLEMENTATION
#include "debug-draw/debug_draw.hpp"
#include <d3dx9math.h>

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZW|D3DFVF_DIFFUSE)

glm::mat4x4 g_vp{};

namespace dd {
    class RenderInterfaceD3D9 final : public dd::RenderInterface {
    public:
        RenderInterfaceD3D9() = delete;
        RenderInterfaceD3D9(const RenderInterfaceD3D9&) = delete;  // Copy constructor
        RenderInterfaceD3D9& operator=(const RenderInterfaceD3D9&) = delete;  // Copy assignment operator
        RenderInterfaceD3D9(RenderInterfaceD3D9&&) = delete;  // Move constructor
        RenderInterfaceD3D9& operator=(RenderInterfaceD3D9&&) = delete;  // Move assignment operator

        RenderInterfaceD3D9(IDirect3DDevice9* device) {
            m_ri_data = new RenderInterfaceData;
            m_ri_data->pd3dDevice = device;
            m_ri_data->pd3dDevice->AddRef();
        };

        ~RenderInterfaceD3D9() {
            if (m_ri_data->pd3dDevice) { m_ri_data->pd3dDevice->Release(); }
            delete m_ri_data;
        }

        D3DMATRIX ConvertToD3DMATRIX(const glm::mat4x4& glm_matrix) {
            D3DMATRIX d3dMatrix;

            auto mat = glm_matrix;//glm::transpose(glm_matrix);
            // Transpose the GLM matrix to match D3D's row-major layout
            memcpy(&d3dMatrix, &mat, sizeof(D3DMATRIX));
#if 0
            d3dMatrix._11 = glm_matrix[0][0]; d3dMatrix._12 = glm_matrix[1][0]; d3dMatrix._13 = glm_matrix[2][0]; d3dMatrix._14 = glm_matrix[3][0];
            d3dMatrix._21 = glm_matrix[0][1]; d3dMatrix._22 = glm_matrix[1][1]; d3dMatrix._23 = glm_matrix[2][1]; d3dMatrix._24 = glm_matrix[3][1];
            d3dMatrix._31 = glm_matrix[0][2]; d3dMatrix._32 = glm_matrix[1][2]; d3dMatrix._33 = glm_matrix[2][2]; d3dMatrix._34 = glm_matrix[3][2];
            d3dMatrix._41 = glm_matrix[0][3]; d3dMatrix._42 = glm_matrix[1][3]; d3dMatrix._43 = glm_matrix[2][3]; d3dMatrix._44 = glm_matrix[3][3];
#endif

            return d3dMatrix;
        }

        void beginDraw() override {
            auto bd = m_ri_data;

            // Backup the DX9 state
            if (bd->pd3dDevice->CreateStateBlock(D3DSBT_ALL, &m_d3d9_state_block) < 0)
                return;
            if (m_d3d9_state_block->Capture() < 0)
            {
                m_d3d9_state_block->Release();
                return;
            }

            D3DVIEWPORT9 vp{};
            vp.X = vp.Y = 0;
            glm::vec2 screen = glm::vec2(devil4_sdk::get_sRender()->screenRes);
            vp.Width = (DWORD)screen.x;
            vp.Height = (DWORD)screen.y;
            vp.MinZ = 0.0f;
            vp.MaxZ = 1.0f;
            bd->pd3dDevice->SetViewport(&vp);

            // Setup render state: fixed-pipeline, alpha-blending, no face culling, no depth testing, shade mode (for gradient), bilinear sampling.
            bd->pd3dDevice->SetPixelShader(nullptr);
            bd->pd3dDevice->SetVertexShader(nullptr);
            
            bd->pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
            bd->pd3dDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
            bd->pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            bd->pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
            bd->pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
            bd->pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
            bd->pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
            bd->pd3dDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
            bd->pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            bd->pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
            bd->pd3dDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
            bd->pd3dDevice->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
            bd->pd3dDevice->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA);
            bd->pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
            bd->pd3dDevice->SetRenderState(D3DRS_FOGENABLE, FALSE);
            bd->pd3dDevice->SetRenderState(D3DRS_RANGEFOGENABLE, FALSE);
            bd->pd3dDevice->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
            bd->pd3dDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
            bd->pd3dDevice->SetRenderState(D3DRS_CLIPPING, TRUE);
            bd->pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
            bd->pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
            bd->pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            bd->pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
            bd->pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
            bd->pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
            bd->pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
            bd->pd3dDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
            bd->pd3dDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
            bd->pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
            bd->pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

            D3DXMATRIX view = ConvertToD3DMATRIX(g_vp);
            bd->pd3dDevice->SetTransform(D3DTS_PROJECTION, &view);
        }

        void endDraw() override {
            // Restore the DX9 state
            m_d3d9_state_block->Apply();
            m_d3d9_state_block->Release();
        }

        void drawLineList(const DrawVertex* lines, int count, bool depthEnabled) {
            (void)depthEnabled;
            // Create and grow buffers if needed
            auto bd = m_ri_data;
            if (!bd->pVB || bd->VertexBufferSize < count)
            {
                if (bd->pVB) { bd->pVB->Release(); bd->pVB = nullptr; }
                bd->VertexBufferSize = count + 5000;
                if (bd->pd3dDevice->CreateVertexBuffer(count * sizeof(CUSTOMVERTEX), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &bd->pVB, nullptr) < 0)
                    return;
            }

            // Allocate buffers
            CUSTOMVERTEX* vtx_dst;
            unsigned short* idx_dst;
            if (bd->pVB->Lock(0, (UINT)(count * sizeof(CUSTOMVERTEX)), (void**)&vtx_dst, D3DLOCK_DISCARD) < 0)
            {
                //d3d9_state_block->Release();
                return;
            }
            for (int n = 0; n < count; n++)
            {
                vtx_dst[n].pos[0] = lines[n].line.x;
                vtx_dst[n].pos[1] = lines[n].line.y;
                vtx_dst[n].pos[2] = lines[n].line.z;
                vtx_dst[n].pos[3] = 1.0f;
                vtx_dst[n].col = D3DCOLOR_ARGB(
                    255,
                    (uint8_t)((lines[n].line.r) * 255.0),
                    (uint8_t)((lines[n].line.g) * 255.0),
                    (uint8_t)((lines[n].line.b) * 255.0));
            }
            bd->pVB->Unlock();
            //bd->pIB->Unlock();
            bd->pd3dDevice->SetStreamSource(0, bd->pVB, 0, sizeof(CUSTOMVERTEX));
            //bd->pd3dDevice->SetIndices(bd->pIB);
            bd->pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);

            bd->pd3dDevice->DrawPrimitive(D3DPT_LINELIST, 0, count / 2);
        }
        
    private:
        // DirectX data
        struct RenderInterfaceData
        {
            LPDIRECT3DDEVICE9           pd3dDevice;
            LPDIRECT3DVERTEXBUFFER9     pVB;
            LPDIRECT3DINDEXBUFFER9      pIB;
            LPDIRECT3DTEXTURE9          FontTexture;
            int                         VertexBufferSize;
            int                         IndexBufferSize;

            RenderInterfaceData() { memset((void*)this, 0, sizeof(*this)); VertexBufferSize = 5000; IndexBufferSize = 10000; }
        };
        struct CUSTOMVERTEX
        {
            float    pos[4];
            D3DCOLOR col;
            //float    uv[2];
        };

        RenderInterfaceData* m_ri_data{ nullptr };
        IDirect3DStateBlock9* m_d3d9_state_block{ nullptr };
    };
} // dd

static dd::RenderInterfaceD3D9* g_debugdraw {nullptr};

namespace w2s {
    glm::vec2 WorldToScreen(const glm::vec3& worldPos) {
        glm::vec2 screen = glm::vec2(devil4_sdk::get_sRender()->screenRes);
        glm::vec4 clipPos = g_vp * glm::vec4(worldPos, 1.0f);
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

#if 0
        ImColor imcol(color);
        float col[] = {
            imcol.Value.x,
            imcol.Value.y,
            imcol.Value.z,
        };

        glm::vec3 vertices[8];
        float halfSize = size / 2.0f;

        vertices[0] = center + glm::vec3(-halfSize, -halfSize, -halfSize);
        vertices[1] = center + glm::vec3(halfSize, -halfSize, -halfSize);
        vertices[2] = center + glm::vec3(halfSize, halfSize, -halfSize);
        vertices[3] = center + glm::vec3(-halfSize, halfSize, -halfSize);
        vertices[4] = center + glm::vec3(-halfSize, -halfSize, halfSize);
        vertices[5] = center + glm::vec3(halfSize, -halfSize, halfSize);
        vertices[6] = center + glm::vec3(halfSize, halfSize, halfSize);
        vertices[7] = center + glm::vec3(-halfSize, halfSize, halfSize);

        dd::box((ddVec3*)&vertices, (float*)col);
        return;
#else
#endif
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
#if 0
        ImColor imcol(color);
        float col[] = {
            imcol.Value.x,
            imcol.Value.y,
            imcol.Value.z,
        };
        ddVec3_In pos { center.x,center.y,center.z };
        dd::sphere(pos, col, radius);
        return;
#endif

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
            ImColor col(color);

#if 0
            dd::line((float*)&rotatedXY1, (float*)&rotatedXY2,(float*)&col);
            dd::line((float*)&rotatedXZ1, (float*)&rotatedXZ2,(float*)&col);
            dd::line((float*)&rotatedYZ1, (float*)&rotatedYZ2,(float*)&col);
#else

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
#endif
        }
    }

    // Draw a wireframe capsule with full XYZ rotation support (based at one end)
    void DrawWireframeCapsule(const glm::vec3& basePosition, float radius, float height, float rotationX, float rotationY, float rotationZ, ImU32 color, int segments, float thickness) {
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();

        // Create rotation matrices for each axis
        glm::mat4 rotationMatrixX = glm::rotate(glm::mat4(1.0f), rotationX, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 rotationMatrixY = glm::rotate(glm::mat4(1.0f), rotationY, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 rotationMatrixZ = glm::rotate(glm::mat4(1.0f), rotationZ, glm::vec3(0.0f, 0.0f, 1.0f));

        // Combine rotation matrices (order: Z * Y * X)
        glm::mat4 rotationMatrix = rotationMatrixZ * rotationMatrixY * rotationMatrixX;

        // Define capsule in local space with origin at base
        glm::vec3 localCenter(0.0f, height * 0.5f, 0.0f);  // Center of capsule in local space
        glm::vec3 topCenter(0.0f, height, 0.0f);           // Top hemisphere center in local space
        glm::vec3 bottomCenter(0.0f, 0.0f, 0.0f);          // Bottom hemisphere center in local space

        // Draw the cylinder part (vertical lines connecting the hemispheres)
        for (int i = 0; i < segments; i++) {
            float angle = 2.0f * glm::pi<float>() * i / segments;

            // Calculate points on the top and bottom circles of the cylinder
            glm::vec3 topPoint(radius * cos(angle), height, radius * sin(angle));
            glm::vec3 bottomPoint(radius * cos(angle), 0.0f, radius * sin(angle));

            // Transform points with rotation
            glm::vec3 rotatedTopPoint = glm::vec3(rotationMatrix * glm::vec4(topPoint, 1.0f));
            glm::vec3 rotatedBottomPoint = glm::vec3(rotationMatrix * glm::vec4(bottomPoint, 1.0f));

            // Translate to base position
            rotatedTopPoint += basePosition;
            rotatedBottomPoint += basePosition;

            // Convert to screen space
            glm::vec2 screenTopPoint = w2s::WorldToScreen(rotatedTopPoint);
            glm::vec2 screenBottomPoint = w2s::WorldToScreen(rotatedBottomPoint);

            // Draw the vertical line
            drawList->AddLine(ImVec2(screenTopPoint.x, screenTopPoint.y), 
                             ImVec2(screenBottomPoint.x, screenBottomPoint.y), 
                             color, thickness);
        }

        // Draw the top and bottom circles of the cylinder
        for (int i = 0; i < segments; i++) {
            float angle = 2.0f * glm::pi<float>() * i / segments;
            float nextAngle = 2.0f * glm::pi<float>() * (i + 1) / segments;

            // Calculate points on the top circle
            glm::vec3 topPoint1(radius * cos(angle), height, radius * sin(angle));
            glm::vec3 topPoint2(radius * cos(nextAngle), height, radius * sin(nextAngle));

            // Calculate points on the bottom circle
            glm::vec3 bottomPoint1(radius * cos(angle), 0.0f, radius * sin(angle));
            glm::vec3 bottomPoint2(radius * cos(nextAngle), 0.0f, radius * sin(nextAngle));

            // Transform points with rotation
            glm::vec3 rotatedTopPoint1 = glm::vec3(rotationMatrix * glm::vec4(topPoint1, 1.0f));
            glm::vec3 rotatedTopPoint2 = glm::vec3(rotationMatrix * glm::vec4(topPoint2, 1.0f));
            glm::vec3 rotatedBottomPoint1 = glm::vec3(rotationMatrix * glm::vec4(bottomPoint1, 1.0f));
            glm::vec3 rotatedBottomPoint2 = glm::vec3(rotationMatrix * glm::vec4(bottomPoint2, 1.0f));

            // Translate to base position
            rotatedTopPoint1 += basePosition;
            rotatedTopPoint2 += basePosition;
            rotatedBottomPoint1 += basePosition;
            rotatedBottomPoint2 += basePosition;

            // Convert to screen space
            glm::vec2 screenTopPoint1 = w2s::WorldToScreen(rotatedTopPoint1);
            glm::vec2 screenTopPoint2 = w2s::WorldToScreen(rotatedTopPoint2);
            glm::vec2 screenBottomPoint1 = w2s::WorldToScreen(rotatedBottomPoint1);
            glm::vec2 screenBottomPoint2 = w2s::WorldToScreen(rotatedBottomPoint2);

            // Draw the horizontal lines
            drawList->AddLine(ImVec2(screenTopPoint1.x, screenTopPoint1.y), 
                              ImVec2(screenTopPoint2.x, screenTopPoint2.y), 
                              color, thickness);
            drawList->AddLine(ImVec2(screenBottomPoint1.x, screenBottomPoint1.y), 
                              ImVec2(screenBottomPoint2.x, screenBottomPoint2.y), 
                              color, thickness);
        }

        // Draw the hemispheres caps (partial circles in two additional planes)
        for (int i = 0; i < segments / 2; i++) {
            float angle = glm::pi<float>() * i / (segments / 2);
            float nextAngle = glm::pi<float>() * (i + 1) / (segments / 2);

            // Top hemisphere - XY plane (front to back)
            glm::vec3 topFrontXY1(radius * cos(angle), height + radius * sin(angle), 0);
            glm::vec3 topFrontXY2(radius * cos(nextAngle), height + radius * sin(nextAngle), 0);

            // Top hemisphere - YZ plane (side to side)
            glm::vec3 topSideYZ1(0, height + radius * sin(angle), radius * cos(angle));
            glm::vec3 topSideYZ2(0, height + radius * sin(nextAngle), radius * cos(nextAngle));

            // Bottom hemisphere - XY plane (front to back)
            glm::vec3 bottomFrontXY1(radius * cos(angle), -radius * sin(angle), 0);
            glm::vec3 bottomFrontXY2(radius * cos(nextAngle), -radius * sin(nextAngle), 0);

            // Bottom hemisphere - YZ plane (side to side)
            glm::vec3 bottomSideYZ1(0, -radius * sin(angle), radius * cos(angle));
            glm::vec3 bottomSideYZ2(0, -radius * sin(nextAngle), radius * cos(nextAngle));

            // Transform all points with rotation
            glm::vec3 rotatedTopFrontXY1 = glm::vec3(rotationMatrix * glm::vec4(topFrontXY1, 1.0f));
            glm::vec3 rotatedTopFrontXY2 = glm::vec3(rotationMatrix * glm::vec4(topFrontXY2, 1.0f));
            glm::vec3 rotatedTopSideYZ1 = glm::vec3(rotationMatrix * glm::vec4(topSideYZ1, 1.0f));
            glm::vec3 rotatedTopSideYZ2 = glm::vec3(rotationMatrix * glm::vec4(topSideYZ2, 1.0f));
            glm::vec3 rotatedBottomFrontXY1 = glm::vec3(rotationMatrix * glm::vec4(bottomFrontXY1, 1.0f));
            glm::vec3 rotatedBottomFrontXY2 = glm::vec3(rotationMatrix * glm::vec4(bottomFrontXY2, 1.0f));
            glm::vec3 rotatedBottomSideYZ1 = glm::vec3(rotationMatrix * glm::vec4(bottomSideYZ1, 1.0f));
            glm::vec3 rotatedBottomSideYZ2 = glm::vec3(rotationMatrix * glm::vec4(bottomSideYZ2, 1.0f));

            // Translate to base position
            rotatedTopFrontXY1 += basePosition;
            rotatedTopFrontXY2 += basePosition;
            rotatedTopSideYZ1 += basePosition;
            rotatedTopSideYZ2 += basePosition;
            rotatedBottomFrontXY1 += basePosition;
            rotatedBottomFrontXY2 += basePosition;
            rotatedBottomSideYZ1 += basePosition;
            rotatedBottomSideYZ2 += basePosition;

            // Convert to screen space
            glm::vec2 screenTopFrontXY1 = w2s::WorldToScreen(rotatedTopFrontXY1);
            glm::vec2 screenTopFrontXY2 = w2s::WorldToScreen(rotatedTopFrontXY2);
            glm::vec2 screenTopSideYZ1 = w2s::WorldToScreen(rotatedTopSideYZ1);
            glm::vec2 screenTopSideYZ2 = w2s::WorldToScreen(rotatedTopSideYZ2);
            glm::vec2 screenBottomFrontXY1 = w2s::WorldToScreen(rotatedBottomFrontXY1);
            glm::vec2 screenBottomFrontXY2 = w2s::WorldToScreen(rotatedBottomFrontXY2);
            glm::vec2 screenBottomSideYZ1 = w2s::WorldToScreen(rotatedBottomSideYZ1);
            glm::vec2 screenBottomSideYZ2 = w2s::WorldToScreen(rotatedBottomSideYZ2);

            // Draw the hemisphere lines
            drawList->AddLine(ImVec2(screenTopFrontXY1.x, screenTopFrontXY1.y), 
                             ImVec2(screenTopFrontXY2.x, screenTopFrontXY2.y), 
                             color, thickness);
            drawList->AddLine(ImVec2(screenTopSideYZ1.x, screenTopSideYZ1.y), 
                             ImVec2(screenTopSideYZ2.x, screenTopSideYZ2.y), 
                             color, thickness);
            drawList->AddLine(ImVec2(screenBottomFrontXY1.x, screenBottomFrontXY1.y), 
                             ImVec2(screenBottomFrontXY2.x, screenBottomFrontXY2.y), 
                             color, thickness);
            drawList->AddLine(ImVec2(screenBottomSideYZ1.x, screenBottomSideYZ1.y), 
                             ImVec2(screenBottomSideYZ2.x, screenBottomSideYZ2.y), 
                             color, thickness);
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

    int dd_init(IDirect3DDevice9* device) {
        g_debugdraw = new dd::RenderInterfaceD3D9(device);
        if(!dd::initialize(g_debugdraw)) { return -1; }
        return 0;
    }

    void dd_update() {
        SMediator* sMed = devil4_sdk::get_sMediator();
        if (!sMed) { return; }
        uCameraCtrl* camera = sMed->camera1;
        if (!camera) { return; }
        glm::mat4 viewMatrix = glm::lookAt(camera->pos, camera->lookat, camera->up);
        glm::vec2 screen = glm::vec2(devil4_sdk::get_sRender()->screenRes);
        float aspectRatio = screen.x / screen.y;
        glm::mat4 projMatrix = glm::perspective(glm::radians(camera->FOV), aspectRatio, 0.1f, 9999.0f);
        g_vp = projMatrix * viewMatrix;

        auto player = devil4_sdk::get_local_player();
        if (!player) { return; }

        ddVec3 position{ player->m_pos.x, player->m_pos.y + 55.0f, player->m_pos.z };
    }

    void dd_flush() {
        dd::flush();
    }

    void dd_shutdown() {
        dd::shutdown();
        delete g_debugdraw;
    }
}