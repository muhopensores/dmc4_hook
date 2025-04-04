#pragma once
#include "sdk/ReClass.hpp"
#include "sdk/Devil4.hpp"
#include "imgui/imgui.h"
#include "utility/Dx9Utils.hpp"

namespace w2s {
	glm::vec2 WorldToScreen(const glm::vec3& targetPos);
	float GetDistanceFromCam(const glm::vec3& targetPos);
	void DrawWireframeCube(const glm::vec3& center, float size, float rotation, ImU32 color, float thickness = 1.0f);
	void DrawWireframeSphere(const glm::vec3& center, float radius, float rotation, ImU32 color, int segments = 16, float thickness = 1.0f);
	void DrawWireframeCapsule(const glm::vec3& center, float radius, float height, float rotationX, float rotationY, float rotationZ, ImU32 color, int segments, float thickness);
	void DrawWireframeCapsule(const glm::vec3& center, float radius, float height, const glm::vec3& rotation, ImU32 color, int segments, float thickness);
	void DrawWireframeCapsule(const glm::vec3& center, float radius, float height, const glm::quat& quaternion, ImU32 color, int segments, float thickness);
	void DrawLine3D(const glm::vec3& start, const glm::vec3& end, ImU32 color, float thickness = 1.0f);
	glm::mat4 CreateRotationMatrix(float pitch, float yaw, float roll);

	void DrawWireframeTriangle(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, ImU32 color, float thickness = 1.0f);
	void DrawFilledTriangle(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, ImU32 color);
	void DrawWireframeMesh(const std::vector<glm::vec3>& vertices, const std::vector<std::vector<int>>& triangles, ImU32 color, float thickness);
	void DrawWireframeMeshIndices(const std::vector<glm::vec3>& vertices, const std::vector<int>& indices, ImU32 color, float thickness = 1.0f);

	bool IsVisibleOnScreen(const glm::vec3& worldPos, float objectRadius = 0.0f);
	// void ScreenToRay(const glm::vec2& screenPos, glm::vec3& rayOrigin, glm::vec3& rayDir);
	// bool RayPlaneIntersection(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& planePos, const glm::vec3& planeNormal, float& t);

	struct TexturedVertex {
		D3DXVECTOR3 position;
		D3DXVECTOR2 texcoord;
		static const DWORD FVF = D3DFVF_XYZ | D3DFVF_TEX1;
	};
	void DrawTextureWithScreenCorners(LPDIRECT3DDEVICE9 device, LPDIRECT3DTEXTURE9 texture, const glm::vec2 corners[4]);
	void DrawTextureWithPerspective(LPDIRECT3DDEVICE9 g_pd3dDevice, PDIRECT3DTEXTURE9 m_texture_handle, const glm::vec3& worldPos, float width, float height, const glm::mat4& viewMatrix, float yRotationDegrees = 0.0f);
};
