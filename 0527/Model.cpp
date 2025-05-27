#include <common.h>
#include "Model.h"
// Graphic.h 已经在 Model.h 中包含

#include <string> // 用于 std::to_string, std::string
#include <vector> // 用于 std::vector
#include <locale> // for tolower
#include <iostream> // 添加 iostream 用于 std::cout, std::cerr, std::endl, std::hex, std::dec
#include <algorithm> // 用于 std::replace


// 构造函数
Model::Model(const std::string& name) :
        GameObject(name),
        m_pMesh(nullptr),
        m_dwNumMaterials(0),
        m_modelType(ModelType::NONE),
        m_pFbxManager(nullptr),
        m_pFbxScene(nullptr) {
    m_vMaterials.clear();
    m_vTextures.clear();
    // std::cout << "DEBUG Model Constructor: Model '" << name << "' created." << std::endl; // 精简日志
}

// 析构函数
Model::~Model() {
    // std::cout << "DEBUG Model Destructor: Releasing resources for model '" << this->name << "'..." << std::endl; // 精简日志
    ReleaseResources();
}

// 释放所有已分配的资源
void Model::ReleaseResources() {
    if (m_pMesh) {
        m_pMesh->Release();
        m_pMesh = nullptr;
    }
    for (size_t i = 0; i < m_vTextures.size(); ++i) {
        if (m_vTextures[i].pTex) {
            m_vTextures[i].pTex->Release();
            m_vTextures[i].pTex = nullptr;
        }
    }
    m_vTextures.clear();
    m_vMaterials.clear();
    m_dwNumMaterials = 0;
    m_modelType = ModelType::NONE;
    m_filePath = "";
    // 清理FBX资源
    CleanupFBX();

    // 清理FBX转换后的网格
    for (auto& mesh : m_vFBXMeshes)
    {
        if (mesh)
        {
            mesh->Release();
            mesh = nullptr;
        }
    }
    m_vFBXMeshes.clear();
    // std::cout << "DEBUG ReleaseResources: All resources cleared." << std::endl; // 精简日志
}

// 从 .X 文件加载模型 (保留了较详细的日志，因为这不是当前出问题的路径)
bool Model::LoadXModelFromFile(const std::string& modelFilePath) {
    std::cout << "DEBUG: --- ENTERING LoadXModelFromFile --- Path: " << modelFilePath << std::endl;
    ReleaseResources();
    m_filePath = modelFilePath;
    m_modelType = ModelType::X_MODEL;

    LPDIRECT3DDEVICE9 pd3dDevice = CGraphic::GetSingleObjPtr()->m_pDevice;
    if (!pd3dDevice) {
        std::cerr << "ERROR LoadXModelFromFile: No D3D Device found for path: " << modelFilePath << std::endl;
        return false;
    }

    LPD3DXBUFFER pD3DXMtrlBuffer = nullptr;
    HRESULT hr = D3DXLoadMeshFromXA(
            modelFilePath.c_str(), D3DXMESH_SYSTEMMEM, pd3dDevice,
            NULL, &pD3DXMtrlBuffer, NULL, &m_dwNumMaterials, &m_pMesh);

    if (FAILED(hr)) {
        std::cerr << "ERROR LoadXModelFromFile: Failed to load .X model: " << modelFilePath << ". HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
        if (pD3DXMtrlBuffer) pD3DXMtrlBuffer->Release();
        return false;
    }
    std::cout << "DEBUG LoadXModelFromFile: D3DXLoadMeshFromX successful. NumMaterials from X file: " << m_dwNumMaterials << std::endl;

    if (m_dwNumMaterials == 0 && m_pMesh != nullptr) {
        m_dwNumMaterials = 1;
        D3DMATERIAL9 defaultMat; ZeroMemory(&defaultMat, sizeof(D3DMATERIAL9));
        defaultMat.Diffuse.r = defaultMat.Ambient.r = 0.8f; defaultMat.Diffuse.g = defaultMat.Ambient.g = 0.8f;
        defaultMat.Diffuse.b = defaultMat.Ambient.b = 0.8f; defaultMat.Diffuse.a = defaultMat.Ambient.a = 1.0f;
        m_vMaterials.push_back(defaultMat); m_vTextures.resize(1);
    } else if (pD3DXMtrlBuffer != nullptr && m_dwNumMaterials > 0) {
        D3DXMATERIAL* d3dxMaterials = (D3DXMATERIAL*)pD3DXMtrlBuffer->GetBufferPointer();
        m_vMaterials.resize(m_dwNumMaterials); m_vTextures.resize(m_dwNumMaterials);
        std::string modelDir = "";
        size_t lastSlash = modelFilePath.find_last_of("/\\");
        if (lastSlash != std::string::npos) modelDir = modelFilePath.substr(0, lastSlash + 1);

        for (DWORD i = 0; i < m_dwNumMaterials; i++) {
            m_vMaterials[i] = d3dxMaterials[i].MatD3D; m_vMaterials[i].Ambient = m_vMaterials[i].Diffuse;
            if (d3dxMaterials[i].pTextureFilename && strlen(d3dxMaterials[i].pTextureFilename) > 0) {
                std::string textureFilenameStr = d3dxMaterials[i].pTextureFilename;
                std::string texturePathStr = modelDir + textureFilenameStr;

                TSTRING path=multi_byte_to_wide_char(texturePathStr.c_str(), 0);
				// 确保路径和文件名都转换为小写
                HRESULT hr_tex_load = CGraphic::GetSingleObjPtr()->LoadTex(path.c_str(), m_vTextures[i], 0);
                if (FAILED(hr_tex_load)) {
                    hr_tex_load = CGraphic::GetSingleObjPtr()->LoadTex(path.c_str(), m_vTextures[i], 0);
                }
                if (SUCCEEDED(hr_tex_load)) {
                    std::cout << "  LoadX: MatIdx " << i << " SUCCESS loading texture: " << (m_vTextures[i].pTex ? textureFilenameStr : "nullptr") << " -> Ptr: " << m_vTextures[i].pTex << std::endl;
                } else {
                    std::cerr << "  LoadX: MatIdx " << i << " FAILED loading texture: " << textureFilenameStr << " (HRESULT: 0x" << std::hex << hr_tex_load << std::dec << ")" << std::endl;
                }
            }
        }
    }
    if (pD3DXMtrlBuffer) pD3DXMtrlBuffer->Release();
    if (m_pMesh) { m_modelType = ModelType::X_MODEL; return true; }
    return false;
}

bool Model::LoadFBXModelFromFile(const std::string& modelFilePath)
{
    // 清理之前的资源
    ReleaseResources();

    m_filePath = modelFilePath;
    m_modelType = ModelType::FBX_MODEL;

    // 初始化FBX SDK
    if (!InitializeFBX())
        return false;

    // 创建FBX导入器
    FbxImporter* pImporter = FbxImporter::Create(m_pFbxManager, "");
    if (!pImporter)
        return false;

    // 初始化导入器
    if (!pImporter->Initialize(modelFilePath.c_str(), -1, m_pFbxManager->GetIOSettings()))
    {
        pImporter->Destroy();
        return false;
    }

    // 创建场景
    m_pFbxScene = FbxScene::Create(m_pFbxManager, "FBX Scene");
    if (!m_pFbxScene)
    {
        pImporter->Destroy();
        return false;
    }

    // 导入场景
    if (!pImporter->Import(m_pFbxScene))
    {
        pImporter->Destroy();
        return false;
    }

    // 转换坐标系（FBX使用右手坐标系，DirectX使用左手坐标系）
	    FbxAxisSystem directXAxisSystem(
		FbxAxisSystem::eYAxis,    // 上方向
		FbxAxisSystem::eParityOdd,// 奇偶性
		FbxAxisSystem::eRightHanded // 右手坐标系
	);
	directXAxisSystem.ConvertScene(m_pFbxScene);

    // 转换FBX数据到DirectX格式
    if (!ConvertFBXToD3D(CGraphic::GetSingleObjPtr()->m_pDevice))
    {
        pImporter->Destroy();
        return false;
    }

    pImporter->Destroy();
    return true;
}

bool Model::InitializeFBX()
{
    m_pFbxManager = FbxManager::Create();
    if (!m_pFbxManager)
        return false;

    FbxIOSettings* pIOSettings = FbxIOSettings::Create(m_pFbxManager, IOSROOT);
    m_pFbxManager->SetIOSettings(pIOSettings);

    return true;
}

void Model::CleanupFBX()
{
    if (m_pFbxScene)
    {
        m_pFbxScene->Destroy();
        m_pFbxScene = nullptr;
    }

    if (m_pFbxManager)
    {
        m_pFbxManager->Destroy();
        m_pFbxManager = nullptr;
    }
}

bool Model::ConvertFBXToD3D(LPDIRECT3DDEVICE9 pd3dDevice)
{
    if (!m_pFbxScene)
        return false;

    // 处理场景中的每个节点
    FbxNode* pRootNode = m_pFbxScene->GetRootNode();
    if (pRootNode)
    {
        for (int i = 0; i < pRootNode->GetChildCount(); i++)
        {
            ProcessFBXNode(pRootNode->GetChild(i), pd3dDevice);
        }
    }

    return true;
}

void Model::ProcessFBXNode(FbxNode* pNode, LPDIRECT3DDEVICE9 pd3dDevice)
{
    if (!pNode)
        return;

    // 处理当前节点的网格
    FbxMesh* pMesh = pNode->GetMesh();
    if (pMesh)
    {
        ProcessFBXMesh(pMesh, pd3dDevice);
    }

    // 处理材质
    for (int i = 0; i < pNode->GetMaterialCount(); i++)
    {
        FbxSurfaceMaterial* pMaterial = pNode->GetMaterial(i);
        if (pMaterial)
        {
            ProcessFBXMaterial(pMaterial);
        }
    }

    // 递归处理子节点
    for (int i = 0; i < pNode->GetChildCount(); i++)
    {
        ProcessFBXNode(pNode->GetChild(i), pd3dDevice);
    }
}

void Model::ProcessFBXMesh(FbxMesh* pMesh, LPDIRECT3DDEVICE9 pd3dDevice)
{
    if (!pMesh)
        return;

    std::cout << "Processing FBX Mesh: " << pMesh->GetName() << std::endl;
    std::cout << "Control Points Count: " << pMesh->GetControlPointsCount() << std::endl;
    std::cout << "Polygon Count: " << pMesh->GetPolygonCount() << std::endl;

    // 获取顶点数据
    int vertexCount = pMesh->GetControlPointsCount();
    FbxVector4* pVertices = pMesh->GetControlPoints();

    // 获取法线数据
    FbxLayerElementNormal* pNormals = pMesh->GetLayer(0)->GetNormals();
    if (pNormals)
    {
        std::cout << "Normals found in layer 0" << std::endl;
    }

    // 获取UV数据
    FbxLayerElementUV* pUVs = pMesh->GetLayer(0)->GetUVs();
    if (pUVs)
    {
        std::cout << "UVs found in layer 0" << std::endl;
    }

    // 创建DirectX网格
    LPD3DXMESH pD3DMesh = nullptr;
    
    // 定义顶点格式
    D3DVERTEXELEMENT9 vertexElements[] = {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
        {0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };

    HRESULT hr = D3DXCreateMesh(
        pMesh->GetPolygonCount(),
        vertexCount,
        D3DXMESH_MANAGED,
        vertexElements,
        pd3dDevice,
        &pD3DMesh
    );

    if (SUCCEEDED(hr))
    {
        std::cout << "Successfully created D3DXMesh" << std::endl;

        // 获取顶点缓冲区
        void* pVertices = nullptr;
        pD3DMesh->LockVertexBuffer(0, &pVertices);
        
        // 获取索引缓冲区
        void* pIndices = nullptr;
        pD3DMesh->LockIndexBuffer(0, &pIndices);

        // 填充顶点数据
        struct Vertex {
            D3DXVECTOR3 position;
            D3DXVECTOR3 normal;
            D3DXVECTOR2 texCoord;
        };
        Vertex* pVertexData = (Vertex*)pVertices;
        WORD* pIndexData = (WORD*)pIndices;

        // 遍历所有多边形
        for (int polyIndex = 0; polyIndex < pMesh->GetPolygonCount(); polyIndex++)
        {
            // 获取多边形的顶点数（通常是3，因为是三角形）
            int polySize = pMesh->GetPolygonSize(polyIndex);
            
            // 遍历多边形的每个顶点
            for (int vertIndex = 0; vertIndex < polySize; vertIndex++)
            {
                int controlPointIndex = pMesh->GetPolygonVertex(polyIndex, vertIndex);
                
                // 设置位置
                FbxVector4 vertex = pMesh->GetControlPointAt(controlPointIndex);
                pVertexData[controlPointIndex].position = D3DXVECTOR3(
                    (float)vertex[0],
                    (float)vertex[1],
                    (float)vertex[2]
                );

                // 设置法线
                if (pNormals)
                {
                    FbxVector4 normal;
                    pMesh->GetPolygonVertexNormal(polyIndex, vertIndex, normal);
                    pVertexData[controlPointIndex].normal = D3DXVECTOR3(
                        (float)normal[0],
                        (float)normal[1],
                        (float)normal[2]
                    );
                }
                else
                {
                    pVertexData[controlPointIndex].normal = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
                }

                // 设置UV坐标
                if (pUVs)
                {
                    FbxVector2 uv;
                    bool unmapped;
                    pMesh->GetPolygonVertexUV(polyIndex, vertIndex, pUVs->GetName(), uv, unmapped);
                    pVertexData[controlPointIndex].texCoord = D3DXVECTOR2(
                        (float)uv[0],
                        (float)uv[1]
                    );
                }
                else
                {
                    pVertexData[controlPointIndex].texCoord = D3DXVECTOR2(0.0f, 0.0f);
                }

                // 设置索引
                pIndexData[polyIndex * 3 + vertIndex] = controlPointIndex;
            }
        }

        // 解锁缓冲区
        pD3DMesh->UnlockVertexBuffer();
        pD3DMesh->UnlockIndexBuffer();

        // 计算法线（如果需要）
        D3DXComputeNormals(pD3DMesh, NULL);

        // 设置材质数量
        m_dwNumMaterials = pMesh->GetElementMaterialCount() > 0 ? pMesh->GetElementMaterialCount() : 1;

        m_vFBXMeshes.push_back(pD3DMesh);
        std::cout << "Successfully processed mesh with " << m_dwNumMaterials << " materials" << std::endl;
    }
    else
    {
        std::cerr << "Failed to create D3DXMesh. HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
    }
}

void Model::ProcessFBXMaterial(FbxSurfaceMaterial* pMaterial)
{
    if (!pMaterial)
        return;

    // 创建DirectX材质
    D3DMATERIAL9 material;
    ZeroMemory(&material, sizeof(D3DMATERIAL9));

    // 设置基本材质属性
    material.Diffuse.r = 1.0f;
    material.Diffuse.g = 1.0f;
    material.Diffuse.b = 1.0f;
    material.Diffuse.a = 1.0f;

    material.Ambient.r = 0.2f;
    material.Ambient.g = 0.2f;
    material.Ambient.b = 0.2f;
    material.Ambient.a = 1.0f;

    material.Specular.r = 0.5f;
    material.Specular.g = 0.5f;
    material.Specular.b = 0.5f;
    material.Specular.a = 1.0f;

    material.Power = 20.0f;

    // 处理材质属性
    if (pMaterial->GetClassId().Is(FbxSurfacePhong::ClassId))
    {
        FbxSurfacePhong* pPhong = (FbxSurfacePhong*)pMaterial;
        
        // 设置漫反射颜色
        FbxDouble3 diffuse = pPhong->Diffuse.Get();
        material.Diffuse.r = (float)diffuse[0];
        material.Diffuse.g = (float)diffuse[1];
        material.Diffuse.b = (float)diffuse[2];
        
        // 设置环境光颜色
        FbxDouble3 ambient = pPhong->Ambient.Get();
        material.Ambient.r = (float)ambient[0];
        material.Ambient.g = (float)ambient[1];
        material.Ambient.b = (float)ambient[2];
        
        // 设置镜面反射颜色
        FbxDouble3 specular = pPhong->Specular.Get();
        material.Specular.r = (float)specular[0];
        material.Specular.g = (float)specular[1];
        material.Specular.b = (float)specular[2];
        
        // 设置光泽度
        material.Power = (float)pPhong->Shininess.Get();
    }
    else if (pMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId))
    {
        FbxSurfaceLambert* pLambert = (FbxSurfaceLambert*)pMaterial;
        
        // 设置漫反射颜色
        FbxDouble3 diffuse = pLambert->Diffuse.Get();
        material.Diffuse.r = (float)diffuse[0];
        material.Diffuse.g = (float)diffuse[1];
        material.Diffuse.b = (float)diffuse[2];
        
        // 设置环境光颜色
        FbxDouble3 ambient = pLambert->Ambient.Get();
        material.Ambient.r = (float)ambient[0];
        material.Ambient.g = (float)ambient[1];
        material.Ambient.b = (float)ambient[2];
    }

    m_vMaterials.push_back(material);

    // 处理纹理
    MyImageInfo textureInfo;
    ZeroMemory(&textureInfo, sizeof(MyImageInfo));

    // 获取漫反射纹理
    FbxProperty diffuseProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sDiffuse);
    if (diffuseProperty.IsValid())
    {
        int textureCount = diffuseProperty.GetSrcObjectCount<FbxTexture>();
        for (int i = 0; i < textureCount; i++)
        {
            FbxTexture* pTexture = diffuseProperty.GetSrcObject<FbxTexture>(i);
            if (pTexture)
            {
                FbxFileTexture* pFileTexture = FbxCast<FbxFileTexture>(pTexture);
                if (pFileTexture)
                {
                    const char* texturePath = pFileTexture->GetFileName();
                    if (texturePath)
                    {
                        // 转换路径格式
                        std::string texturePathStr = texturePath;
                        std::replace(texturePathStr.begin(), texturePathStr.end(), '\\', '/');
                        
                        // 加载纹理
                        TSTRING path = multi_byte_to_wide_char(texturePathStr.c_str(), 0);
                        HRESULT hr = CGraphic::GetSingleObjPtr()->LoadTex(path.c_str(), textureInfo, 0);
                        if (SUCCEEDED(hr))
                        {
                            m_vTextures.push_back(textureInfo);
                            break;
                        }
                    }
                }
            }
        }
    }

    // 如果没有找到纹理，添加一个空的纹理信息
    if (m_vTextures.size() < m_vMaterials.size())
    {
        m_vTextures.push_back(textureInfo);
    }
}

// 渲染函数
void Model::Render(LPDIRECT3DDEVICE9 pd3dDevice) {
    if (!pd3dDevice) {
        std::cerr << "ERROR Model::Render: No D3D Device." << std::endl;
        return;
    }

    D3DXMATRIXA16 matWorld = transform.GetWorldMatrix();
    pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);

    if (m_modelType == ModelType::X_MODEL && m_pMesh) {
        for (DWORD i = 0; i < m_dwNumMaterials; i++) {
            if (i < m_vMaterials.size()) pd3dDevice->SetMaterial(&m_vMaterials[i]);
            else if (!m_vMaterials.empty()) pd3dDevice->SetMaterial(&m_vMaterials[0]);
            if (i < m_vTextures.size() && m_vTextures[i].pTex != nullptr) pd3dDevice->SetTexture(0, m_vTextures[i].pTex);
            else pd3dDevice->SetTexture(0, nullptr);
            m_pMesh->DrawSubset(i);
        }
    } else if (m_modelType == ModelType::FBX_MODEL) {
        std::cout << "Rendering FBX Model with " << m_vFBXMeshes.size() << " meshes" << std::endl;
        std::cout << "Number of materials: " << m_dwNumMaterials << std::endl;
        
        // 渲染FBX模型
        for (size_t i = 0; i < m_vFBXMeshes.size(); i++)
        {
            if (m_vFBXMeshes[i])
            {
                std::cout << "Rendering mesh " << i << std::endl;
                
                // 设置材质
                if (i < m_vMaterials.size())
                {
                    std::cout << "Setting material " << i << std::endl;
                    pd3dDevice->SetMaterial(&m_vMaterials[i]);
                }
                else if (!m_vMaterials.empty())
                {
                    std::cout << "Using default material" << std::endl;
                    pd3dDevice->SetMaterial(&m_vMaterials[0]);
                }

                // 设置纹理
                if (i < m_vTextures.size() && m_vTextures[i].pTex)
                {
                    std::cout << "Setting texture " << i << std::endl;
                    pd3dDevice->SetTexture(0, m_vTextures[i].pTex);
                }
                else
                {
                    std::cout << "No texture for mesh " << i << std::endl;
                    pd3dDevice->SetTexture(0, nullptr);
                }

                // 渲染网格
                m_vFBXMeshes[i]->DrawSubset(0);
            }
        }
    }
}