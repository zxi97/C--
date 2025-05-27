#include <head.h>
#include "Graphic.h"
#include "Model.h"
#include "CGameObject.h"
#include "CTransform.h"

// 全局变量
D3DXVECTOR3 vEye{ 0.0f, 0.0f, 100.0f }; // 摄像机位置
D3DXVECTOR3 vAt{ 0.0f, 0.0f, 0.0f };    // 摄像机观察目标
D3DXVECTOR3 vUp{ 0.0f, 1.0f, 0.0f };     // 摄像机向上的向量
D3DXVECTOR3 vOffset{ 0.0f, 0.0f, 100.0f }; // 摄像机相对于模型的偏移量

// 相机旋转角度
float fYaw = D3DX_PI;    // 水平旋转角度（180度）
float fPitch = 0.0f;  // 垂直旋转角度
POINT lastMousePos;   // 上一帧的鼠标位置
bool isFirstMouse = true; // 是否是第一次移动鼠标
bool isLeftButtonDown = false; // 鼠标左键是否按下

Model* g_pModel = nullptr;   // 用于加载 .x 模型的指针

// 窗口事件处理函数
LRESULT EventProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_DESTROY: // 窗口被销毁
            if (g_pModel)
            {
                delete g_pModel;
                g_pModel = nullptr;
            }
            PostQuitMessage(0);
            break;
            
        case WM_KEYDOWN: // 键盘按下事件
            // 简单的摄像机控制
            if (wParam == 'A') { vOffset.x -= 1.5f; }
            if (wParam == 'D') { vOffset.x += 1.5f; }
            if (wParam == 'W') { vOffset.z += 1.5f; }
            if (wParam == 'S') { vOffset.z -= 1.5f; }
            if (wParam == 'Q') { vOffset.y += 1.5f; }
            if (wParam == 'E') { vOffset.y -= 1.5f; }
            break;

        case WM_LBUTTONDOWN: // 鼠标左键按下
            isLeftButtonDown = true;
            isFirstMouse = true; // 重置第一次移动标记
            break;

        case WM_LBUTTONUP: // 鼠标左键释放
            isLeftButtonDown = false;
            break;

        case WM_MOUSEMOVE: // 鼠标移动事件
        {
            // 只在按住左键时处理相机旋转
            if (!isLeftButtonDown)
                break;

            POINT currentMousePos;
            currentMousePos.x = LOWORD(lParam);
            currentMousePos.y = HIWORD(lParam);

            if (isFirstMouse)
            {
                lastMousePos = currentMousePos;
                isFirstMouse = false;
            }

            // 计算鼠标偏移量
            float xOffset = currentMousePos.x - lastMousePos.x;
            float yOffset = lastMousePos.y - currentMousePos.y; // 注意y轴方向是反的
            lastMousePos = currentMousePos;

            // 将偏移量转换为旋转角度
            float sensitivity = 0.005f; // 鼠标灵敏度
            xOffset *= sensitivity;
            yOffset *= sensitivity;
            fYaw += xOffset;
            fPitch += yOffset;

            // 限制垂直旋转角度，防止相机翻转
            if (fPitch > D3DX_PI / 2.0f - 0.1f)
                fPitch = D3DX_PI / 2.0f - 0.1f;
            if (fPitch < -D3DX_PI / 2.0f + 0.1f)
                fPitch = -D3DX_PI / 2.0f + 0.1f;

            break;
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// 主函数
INT _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nShowCmd)
{
    // 1. 初始化窗口
    HWND hwnd = CGraphic::GetSingleObjPtr()->InitWindow(EventProc, 800, 600, _T("DX_3D_Model_Loader"));
    if (!hwnd) {
        MessageBox(NULL, _T("窗口初始化失败!"), _T("错误"), MB_OK | MB_ICONERROR);
        return -1;
    }

    // 2. 初始化Direct3D
    CGraphic::GetSingleObjPtr()->InitDX(hwnd);
    LPDIRECT3DDEVICE9 pd3dDevice = CGraphic::GetSingleObjPtr()->m_pDevice;

    // 3. 创建和加载模型
    g_pModel = new Model("X_Model");
    bool modelLoaded = false;
    
    // 尝试加载 .x 模型
    if (g_pModel->LoadXModelFromFile("./resource/model/binglongabc.x")) {
        modelLoaded = true;
    }
    // 如果 .x 模型加载失败，尝试加载 .fbx 模型
    else if (g_pModel->LoadFBXModelFromFile("./resource/model/juese.fbx")) {
        modelLoaded = true;
    }

    if (modelLoaded) {
        g_pModel->transform.SetPosition(0.0f, 0.0f, 0.0f);
        g_pModel->transform.SetScale(1.0f, 1.0f, 1.0f);
        // 添加初始旋转：先绕Z轴旋转90度，再绕X轴旋转90度，最后绕Y轴旋转180度
        g_pModel->transform.SetRotationEuler(D3DX_PI, -D3DX_PI/2.0f, D3DX_PI/2.0f);
    } else {
        MessageBox(hwnd, _T("错误：无法加载模型！"), _T("模型加载失败"), MB_OK | MB_ICONERROR);
    }

    // 4. 设置光照
    pd3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
    D3DLIGHT9 light;
    ZeroMemory(&light, sizeof(D3DLIGHT9));
    light.Type = D3DLIGHT_DIRECTIONAL;
    light.Diffuse.r = 1.0f; light.Diffuse.g = 1.0f; light.Diffuse.b = 1.0f;
    light.Ambient.r = 0.4f; light.Ambient.g = 0.4f; light.Ambient.b = 0.4f;
    light.Specular.r = 0.7f; light.Specular.g = 0.7f; light.Specular.b = 0.7f;
    D3DXVECTOR3 vecDir(0.5f, -0.8f, 0.75f);
    D3DXVec3Normalize((D3DXVECTOR3*)&light.Direction, &vecDir);
    pd3dDevice->SetLight(0, &light);
    pd3dDevice->LightEnable(0, TRUE);
    pd3dDevice->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(80, 80, 80));
    pd3dDevice->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);

    // 5. 设置渲染状态
    pd3dDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
    pd3dDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

    // 6. 设置投影矩阵
    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4.0f, 800.0f / 600.0f, 1.0f, 2000.0f);
    pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);

    // 7. 进入主循环
    MSG msg;
    ZeroMemory(&msg, sizeof(MSG));
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // 更新逻辑
            if (g_pModel) {
                g_pModel->Update(0.0f);
                
                // 更新摄像机位置，使其跟随模型
                D3DXVECTOR3 modelPos = g_pModel->transform.position;
                
                // 计算相机位置
                D3DXVECTOR3 offset;
                // 先应用水平旋转
                offset.x = vOffset.x * cosf(fYaw) - vOffset.z * sinf(fYaw);
                offset.z = vOffset.x * sinf(fYaw) + vOffset.z * cosf(fYaw);
                offset.y = vOffset.y; // 直接使用vOffset的y值
                
                // 应用垂直旋转
                float horizontalDistance = sqrtf(offset.x * offset.x + offset.z * offset.z);
                float verticalDistance = horizontalDistance * cosf(fPitch);
                offset.y = horizontalDistance * sinf(fPitch) + vOffset.y; // 保持垂直偏移
                
                // 更新相机位置
                vEye = modelPos + offset;
                vAt = modelPos; // 观察目标始终对准模型
            }

            // 设置视图矩阵
            D3DXMATRIX matView;
            D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
            pd3dDevice->SetTransform(D3DTS_VIEW, &matView);

            // 渲染
            CGraphic::GetSingleObjPtr()->BeginDraw();

            if (g_pModel) {
                g_pModel->Render(pd3dDevice);
            }

            CGraphic::GetSingleObjPtr()->EndDraw();
        }
    }

    // 8. 清理资源
    if (g_pModel) {
        delete g_pModel;
        g_pModel = nullptr;
    }

    return 0;
} 