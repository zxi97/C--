#pragma once
#include "CGameObject.h" // 你的游戏对象基类
#include "Graphic.h"     // 包含 CGraphic 和 MyImageInfo 定义
#include <vector>
#include <string>
#include <d3dx9.h>
#include <fbxsdk.h>

class Model : public GameObject {
public:
    Model(const std::string& name);
    virtual ~Model();

    bool LoadXModelFromFile(const std::string& modelFilePath);
    bool LoadFBXModelFromFile(const std::string& modelFilePath);

    void Render(LPDIRECT3DDEVICE9 pd3dDevice);
    void ReleaseResources();

private:
    enum class ModelType {
        NONE,
        X_MODEL,
        FBX_MODEL
    };
    ModelType m_modelType;
    std::string m_filePath;

    // .X 模型数据
    LPD3DXMESH m_pMesh;

    // FBX 模型数据
    FbxManager* m_pFbxManager;
    FbxScene* m_pFbxScene;
    std::vector<LPD3DXMESH> m_vFBXMeshes;  // 存储转换后的FBX网格

    // 通用材质和纹理数据
    DWORD m_dwNumMaterials;
    std::vector<D3DMATERIAL9> m_vMaterials;
    std::vector<MyImageInfo> m_vTextures;

    // FBX 相关辅助函数
    bool InitializeFBX();
    void CleanupFBX();
    bool ConvertFBXToD3D(LPDIRECT3DDEVICE9 pd3dDevice);
    void ProcessFBXNode(FbxNode* pNode, LPDIRECT3DDEVICE9 pd3dDevice);
    void ProcessFBXMesh(FbxMesh* pMesh, LPDIRECT3DDEVICE9 pd3dDevice);
    void ProcessFBXMaterial(FbxSurfaceMaterial* pMaterial);
};