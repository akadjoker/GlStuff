
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "Core.hpp"
#include "Buffer.hpp"
#include "main.h"

int screenWidth = 1024;
int screenHeight = 768;


 


 

void updateCascades(float nearClip, float farClip, const Vec3 & lightPos, const Mat4 & viewMatrix, const Mat4 & projectionMatrix )
	{
		float cascadeSplits[SHADOW_MAP_CASCADE_COUNT];
     	float cascadeSplitLambda = 0.95f;

		float clipRange = farClip - nearClip;

		float minZ = nearClip;
		float maxZ = nearClip + clipRange;

		float range = maxZ - minZ;
		float ratio = maxZ / minZ;

        for (u32 i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
        {
            float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
			float log = minZ * Pow(ratio, p);
			float uniform = minZ + range * p;
			float d = cascadeSplitLambda * (log - uniform) + uniform;
			cascadeSplits[i] = (d - nearClip) / clipRange;
        }
	
		// Calculate orthographic projection matrix for each cascade
		float lastSplitDist = 0.0;
		for (u32 i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) 
        {
			float splitDist = cascadeSplits[i];

			Vec3 frustumCorners[8] = 
            {
				Vec3(-1.0f,  1.0f, 0.0f),
				Vec3( 1.0f,  1.0f, 0.0f),
				Vec3( 1.0f, -1.0f, 0.0f),
				Vec3(-1.0f, -1.0f, 0.0f),
				Vec3(-1.0f,  1.0f,  1.0f),
				Vec3( 1.0f,  1.0f,  1.0f),
				Vec3( 1.0f, -1.0f,  1.0f),
				Vec3(-1.0f, -1.0f,  1.0f),
			};

			// Project frustum corners into world space
			Mat4 invCam =Mat4::Inverse(projectionMatrix * viewMatrix);   
			for (u32 j = 0; j < 8; j++) 
            {
				Vec4 invCorner = invCam * Vec4(frustumCorners[j], 1.0f);
                Vec4 div = invCorner / invCorner.w;
				frustumCorners[j] = Vec3(div.x, div.y, div.z);
			}

			for (u32 j = 0; j < 4; j++) 
            {
				Vec3 dist = frustumCorners[j + 4] - frustumCorners[j];
				frustumCorners[j + 4] = frustumCorners[j] + (dist * splitDist);
				frustumCorners[j] = frustumCorners[j] + (dist * lastSplitDist);
			}

			// Get frustum center
			Vec3 frustumCenter = Vec3(0.0f);
			for (u32 j = 0; j < 8; j++) 
            {
				frustumCenter += frustumCorners[j];
			}
			frustumCenter /= 8.0f;

			float radius = 0.0f;
			for (u32 j = 0; j < 8; j++) 
            {
				float distance = Vec3::Length(frustumCorners[j] - frustumCenter);
				radius = Max(radius, distance);
			}
			radius = Ceil(radius * 16.0f) / 16.0f;

			Vec3 maxExtents = Vec3(radius);
			Vec3 minExtents = -maxExtents;

			Vec3 lightDir = Vec3::Normalize(-lightPos);
			Mat4 lightViewMatrix  = Mat4::LookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, Vec3(0.0f, 1.0f, 0.0f));
			Mat4 lightOrthoMatrix = Mat4::Ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);


      

			// Store split distance and matrix in cascade
			cascades[i].splitDepth = (nearClip + splitDist * clipRange) * -1.0f;
			cascades[i].viewProjMatrix = lightOrthoMatrix * lightViewMatrix;

			lastSplitDist = cascadeSplits[i];
		}
	}
    

void LoadShader()
{
  const char *vShader = GLSL(
 

layout(location=0) in vec3 aPosition;
layout(location=1) in vec2 aTexCoord;
layout(location=2) in vec3 aNormal;
layout(location=3) in vec4 aTangent;    

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec3 outViewPosition;
out vec4 outWorldPosition;
out mediump vec2 vUV;
out highp   mat3 vTBN;

uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;

void main()
{
    mat3 Nmat = transpose(inverse(mat3(model)));

    vec3 N = normalize(Nmat * aNormal);
    vec3 T = normalize(Nmat * aTangent.xyz);
    T = normalize(T - dot(T, N) * N);
    vec3 B = normalize(cross(N, T)) * aTangent.w;

    vTBN = mat3(T, B, N);
    vUV  = aTexCoord;
    TexCoords = aTexCoord;

    vec4 worldPos = model * vec4(aPosition, 1.0);
    FragPos = worldPos.xyz;
    outWorldPosition = worldPos;

    vec4 viewPos4 = view * worldPos;       // nome diferente de uniform do FS
    outViewPosition = viewPos4.xyz;

    gl_Position = proj * viewPos4;
    Normal = N;                             // mantém Normal em WS coerente
}

);

const char *fShader = GLSL(
    const int NUM_CASCADES = 4;

    out vec4 FragColor;

    in vec3 FragPos;
    in vec2 TexCoords;
    in vec3 Normal;
    in vec3 outViewPosition;
    in vec4 outWorldPosition;
    in mediump vec2 vUV;
    in mediump mat3 vTBN;

    struct CascadeShadow 
    {
        mat4 projViewMatrix;
        float splitDistance;
    };

    uniform sampler2D diffuseMap;
    uniform sampler2D normalMap;
    uniform CascadeShadow cascadeshadows[NUM_CASCADES];
    uniform sampler2D shadowMap[NUM_CASCADES];

    uniform vec3 lightDir;
    uniform float farPlane;
    uniform vec3 viewPos;
    uniform float bumpScale;
    uniform float shininess;

    uniform vec2 shadowMapSize;  


    uniform float shadowStrength;  // quanto a sombra pesa (tip: 1.25–2.0)
    uniform float shadowGamma;     // curva na luz visível (tip: 1.2–2.0)

    float CalculateShadow_main(vec4 worldPosition, int idx)
    {
        // proj -> NDC -> [0,1]
        vec4 smp = cascadeshadows[idx].projViewMatrix * worldPosition;
        vec3 projCoords = smp.xyz / smp.w;
        projCoords = projCoords * 0.5 + 0.5;

        // fora do frustum ou atrás do far da luz
        if (projCoords.z > 1.0 ||
            projCoords.x < 0.0 || projCoords.x > 1.0 ||
            projCoords.y < 0.0 || projCoords.y > 1.0)
        {
            return 0.0;
        }
 
        vec2 texelSize = 1.0 / vec2(textureSize(shadowMap[idx], 0));

        vec3 n = normalize(Normal);
        float bias = max(0.05 * (1.0 - dot(n, -lightDir)), 0.005);
        const float biasModifier = 0.5;   

 
        if (idx == NUM_CASCADES - 1)
            bias *= 1.0 / (farPlane * biasModifier);                        
        else
            bias *= 1.0 / (cascadeshadows[idx].splitDistance * biasModifier);

        // PCF 3x3
        float shadow = 0.0;
        for (int x = -1; x <= 1; ++x)
        {
            for (int y = -1; y <= 1; ++y)
            {
                vec2 uv = projCoords.xy + vec2(x, y) * texelSize;
                float pcfDepth = texture(shadowMap[idx], uv).r;
                shadow += (projCoords.z - bias) > pcfDepth ? 1.0 : 0.0;
            }
        }
        shadow /= 9.0;

        return shadow;
    }

 
float PCF5RotatedShadow(int idx, vec2 uv, float refDepth, float bias)
{
    // offset isotrópico por texel (Horde usa 1.0/shadowMapSize)
    float off = 1.0 / max(shadowMapSize.x, shadowMapSize.y);

    // amostras em grelha ~30° (centro + 4 diagonais)
    vec2 o0 = vec2(0.0);
    vec2 o1 = vec2(-0.866,  0.5) * off;
    vec2 o2 = vec2(-0.866, -0.5) * off;
    vec2 o3 = vec2( 0.866, -0.5) * off;
    vec2 o4 = vec2( 0.866,  0.5) * off;

    float s = 0.0;
    float d;

    d = texture(shadowMap[idx], uv + o0).r; s += (refDepth - bias) > d ? 1.0 : 0.0;
    d = texture(shadowMap[idx], uv + o1).r; s += (refDepth - bias) > d ? 1.0 : 0.0;
    d = texture(shadowMap[idx], uv + o2).r; s += (refDepth - bias) > d ? 1.0 : 0.0;
    d = texture(shadowMap[idx], uv + o3).r; s += (refDepth - bias) > d ? 1.0 : 0.0;
    d = texture(shadowMap[idx], uv + o4).r; s += (refDepth - bias) > d ? 1.0 : 0.0;

    return s / 5.0;  // 0=sem sombra, 1=totalmente em sombra
}

 
float CalculateShadow(vec4 worldPosition, int idx)
{
    // projetar para o espaço do shadow map da cascata
    vec4 smp = cascadeshadows[idx].projViewMatrix * worldPosition;
    vec3 proj = smp.xyz / max(smp.w, 1e-6);
    proj = proj * 0.5 + 0.5;  // NDC -> [0,1]

    // fora do frustum da luz ou além do far -> sem sombra
    if (proj.z > 1.0 || proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 || proj.y > 1.0)
        return 0.0;

    // bias (constante + dependente da inclinação) – usa a Normal geométrica
    vec3 n = normalize(Normal);
    vec3 L = normalize(-lightDir);                 // a tua luz é direcional
    float NdotL = max(dot(n, L), 0.0);

    // ajusta estes dois se ainda houver acne/peter panning:
    float constBias = 0.0025;                       // 0.0015–0.02
    float slopeBias = 0.02;                        // 0.01–0.1
    float bias = constBias + slopeBias * (1.0 - NdotL);

    // PCF 5-tap (retorna fração EM SOMBRA, compatível com o teu (1.0 - shadow))
    return PCF5RotatedShadow(idx, proj.xy, proj.z, bias);
}




    void main() 
    {
  
        vec3 albedo = texture(diffuseMap, vUV).rgb;
        vec3 normalTS = texture(normalMap, vUV).xyz * 2.0 - 1.0;
        normalTS.xy *= bumpScale;
        
        // Transform normal from tangent space to world space
        vec3 N = normalize(vTBN * normalTS);
        
        // Light direction (assuming it's in world space)
        vec3 L = normalize(-lightDir);
        vec3 V = normalize(viewPos - FragPos);

        // Calculate cascade index
        int cascadeIndex = 0;
        for (int i = 0; i < NUM_CASCADES - 1; i++) 
        {
            if (-outViewPosition.z < cascadeshadows[i].splitDistance) 
            {
                cascadeIndex = i + 1;
            }
        }

        // Lighting calculations
        vec3 lightColor = vec3(0.9);
        vec3 ambient = 0.02 * albedo;
        
        float NdotL = max(dot(N, L), 0.0);
        vec3 diffuse = NdotL * lightColor * albedo;

        // Specular (Blinn-Phong)
        vec3 H = normalize(L + V);
        float NdotH = max(dot(N, H), 0.0);
        float spec = pow(NdotH, shininess);
        vec3 specular = spec * lightColor;

        // Shadow calculation
        float shadow = CalculateShadow(outWorldPosition, cascadeIndex);
        shadow = clamp(shadow * shadowStrength, 0.0, 1.0);
        float visibility = pow(1.0 - shadow, shadowGamma);

        // sombrear um pouco o ambiente
         float ambientShadow = mix(1.0, 0.5, shadow); // 0.5 = 50% do ambiente nas sombras
         ambient *= ambientShadow;


        // Final lighting
        //vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular);
        vec3 lighting = ambient + visibility * (diffuse + specular);

        FragColor = vec4(lighting, 1.0);
    }


);


 
if (ShaderManager::Instance().Create(vShader, fShader, "shadow"))
{
    Shader * m_modelShader = ShaderManager::Instance().Get("shadow");
    m_modelShader->LoadDefaults();
    
    // Texture units
    m_modelShader->SetInt("diffuseMap", 0);
    m_modelShader->SetInt("normalMap", 1);
    
    // Shadow maps starting from texture unit 2
    for (u32 i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
    {
        m_modelShader->SetInt("shadowMap[" + std::to_string(i) + "]", i + 2);
    }
    
    // Default values
    m_modelShader->SetFloat("bumpScale", 1.0f);
    m_modelShader->SetFloat("shininess", 64.0f);
    m_modelShader->SetFloat("viewPos", 0.0f, 0.0f, 0.0f);
    m_modelShader->SetFloat("lightDir", 0.0f, 0.0f, 0.0f);
    
    // Set shadow map size uniform
//    m_modelShader->SetFloat("shadowMapSize", (float)SHADOW_WIDTH, (float)SHADOW_HEIGHT);
}
}


void LoadDepthShader()
{
    const char* depthVertexShader = GLSL(
    layout(location = 0) in vec3 aPosition;
    
    uniform mat4 lightSpaceMatrix;
    uniform mat4 model;
    
    void main()
    {
        gl_Position = lightSpaceMatrix * model * vec4(aPosition, 1.0);
    }
);

// Depth-only fragment shader
const char* depthFragmentShader = GLSL(
    void main()
    {
        // OpenGL ES automatically writes to gl_FragDepth
        // No need for explicit depth output in ES 3.0
    }
);

if (ShaderManager::Instance().Create(depthVertexShader, depthFragmentShader, "depth"))
{
    Shader* depthShader = ShaderManager::Instance().Get("depth");
    depthShader->LoadDefaults();
}

}

int main()
{

    Device &device = Device::Instance();
    
    if (!device.Create(screenWidth, screenHeight, "Game",true))
    {
        return 1;
    }
    
    LoadShader();
    LoadDepthShader();
    RenderBatch batch;
    batch.Init();
    batch.SetColor(255, 255, 255);
    Mat4  ortho = Mat4::Ortho(0, device.GetWidth(), device.GetHeight(),  0, -1, 1);
    batch.SetMatrix(ortho);

    RenderBatch batch3D;
    batch3D.Init();

    Font font;
    font.LoadDefaultFont();
    font.SetFontSize(16);
    font.SetBatch(&batch);


    Shader* shader = ShaderManager::Instance().Get("shadow");
    Shader* solid = ShaderManager::Instance().Get("Default");
    Texture2D* diffuse =  TextureManager::Instance().Load("Rocks_Diffuse.png", "diffuse");
    Texture2D* normal  =  TextureManager::Instance().Load("Rocks_Normal.png", "normal");

    Texture2D* cubeDiffuse =  TextureManager::Instance().Load("rockwall.tga", "cube_diffuse");
    Texture2D* cubeNormal  =  TextureManager::Instance().Load("rockwall_NH.tga", "cube_normal");

    diffuse->SetAnisotropicFiltering(8.0f);
    
    

    Camera camera;

    Mat4 projection = camera.GetProjectionMatrix((float)screenWidth / (float)screenHeight);
    Mat4 mvp = projection * camera.GetViewMatrix();
    batch3D.SetMatrix(mvp);

    Scene& scene = Scene::Instance();
    


    Mesh* cube = MeshManager::Instance().CreateCube(1.0f);
    Mesh* plane = MeshManager::Instance().CreatePlane(10.0f, 10.0f,16,16, 5, 5);
    plane->SetCastsShadows(false);
    Mesh* sphere = MeshManager::Instance().CreateSphere(0.5f, 16, 16);
    Mesh* cylinder = MeshManager::Instance().CreateCylinder(1.0f, 2.0f, 16);

    Model* floor = scene.CreateModel();
    floor->AddMesh(plane);
    floor->AddMaterial()->SetTexture(0, diffuse)->SetTexture(1, normal);
    

    Model* sphereModel = scene.CreateModel();
    sphereModel->AddMesh(sphere);
    sphereModel->SetPosition(Vec3(0.0f, 1.0f, 2.0f));
    sphereModel->AddMaterial()->SetTexture(0, diffuse)->SetTexture(1, normal);

    Model* cylinderModel = scene.CreateModel();
    cylinderModel->AddMesh(cylinder);
    cylinderModel->SetPosition(Vec3(2.0f, 1.0f, 0.0f));
    cylinderModel->SetScale(Vec3(0.5f, 0.5f, 0.5f));
    cylinderModel->AddMaterial()->SetTexture(0, diffuse)->SetTexture(1, normal);


    Model* cubeModel = scene.CreateModel();
    cubeModel->AddMesh(cube);
    cubeModel->SetPosition(Vec3(-2.0f, 1.0f, -2.0f));
    cubeModel->SetScale(Vec3(0.5f, 0.5f, 0.5f));
    cubeModel->AddMaterial()->SetTexture(0, cubeDiffuse)->SetTexture(1, cubeNormal);


    ShadowMapManager  shadowManager;
    shadowManager.Initialize();
 

    float bump = 0.8f;


    Vec3 lightPosition = Vec3(0.0f, 5.0f, -1.0f);

    while (device.Run())
    {

        const float dt = device.GetFrameTime();
        float SPEED = 0.1f * dt;
        
        if (device.IsResize())
        {
            glViewport(0, 0, device.GetWidth(), device.GetHeight());
            
            Mat4  ortho = Mat4::Ortho(0, device.GetWidth(), device.GetHeight(),  0, -1, 1);
            batch.SetMatrix(ortho);
            
            projection = camera.GetProjectionMatrix((float)screenWidth / (float)screenHeight);
            
            
        }
        Mat4 view = camera.GetViewMatrix();
        mvp = projection * view;
        batch3D.SetMatrix(mvp);

        if (Keyboard::Down(KEY_ENTER))
        {
         
            lightPosition = camera.GetPosition();
        }
        
        camera.MouseView(dt);
        if (Keyboard::Down(KEY_W))
        {
            camera.ProcessKeyboard(0, SPEED);
        } 
        if (Keyboard::Down(KEY_S))
        {
            camera.ProcessKeyboard(1, SPEED);
        }
        if (Keyboard::Down(KEY_A))
        {
            camera.ProcessKeyboard(2, SPEED);
        }
        if (Keyboard::Down(KEY_D))
        {
            camera.ProcessKeyboard(3, SPEED);
        }


        if (Keyboard::Down(KEY_UP))
        {
            bump += 0.01f;
        }
        if (Keyboard::Down(KEY_DOWN))
        {
            bump -= 0.01f;
        }
        
        if (bump < 0.0f)
        {
            bump = 0.0f;
        }
        if (bump > 2.0f)
        {
            bump = 2.0f;
        }
        

        updateCascades(camera.getNearClip(), camera.getFarClip(), lightPosition, view, projection);

        Shader* depthShader = ShaderManager::Instance().Get("depth");
        depthShader->Use();
        for (int i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
        {
            shadowManager.BeginShadowPass(i);
            depthShader->SetMatrix4("lightSpaceMatrix", cascades[i].viewProjMatrix.x);
            scene.RenderDepth(depthShader);
            shadowManager.EndShadowPass();
        }
        glViewport(0,0,device.GetWidth(),device.GetHeight());
        depthShader->Use(false);



        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        cubeModel->Rotate(Vec3(0.0f, 1.0f, 1.0f));



        Mat4 model = Mat4::Identity();

        shader->Use();
        shader->SetMatrix4("model", model.x);
        shader->SetMatrix4("view", view.x);
        shader->SetMatrix4("proj", projection.x);
        shader->SetFloat("bumpScale", bump);
        Vec3 cameraPos = camera.GetPosition();
        shader->SetFloat("viewPos", cameraPos.x, cameraPos.y, cameraPos.z);
        
        Vec3 lightDir = Vec3::Normalize(-lightPosition);
        shader->SetFloat("lightDir", lightDir.x, lightDir.y, lightDir.z);


        //shader->SetFloat("farPlane", camera.getFarClip());
        shadowManager.BindShadowMapsForReading(shader);
        shadowManager.UpdateCascadeUniforms(shader, cascades);

        
        scene.Update(dt);
        scene.SetShader(shader);
        scene.Render();

        shader->Use(false);

        model = Mat4::Translate(0.0f, 1.0f, 0.0f);


        solid->Use();
        solid->SetMatrix4("model", model.x);
        solid->SetMatrix4("view", view.x);
        solid->SetMatrix4("proj", projection.x);
        

        solid->Use(false);


        


        batch3D.SetColor(1.0f, 1.0f, 1.0f);
 
       // batch3D.Grid(10, 10);
        batch3D.Render();

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
       // glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);//good 4 text

        batch.SetColor(1.0f, 1.0f, 1.0f);
        batch.Line2D(Vec2(0, 0), Vec2(100, 100));
        batch.Quad(shadowManager.GetShadowMap(0), 20,20,100,100);
        batch.Quad(shadowManager.GetShadowMap(1), 20,130,100,100);
        batch.Quad(shadowManager.GetShadowMap(2), 20,240,100,100);
        batch.Quad(shadowManager.GetShadowMap(3), 20,350,100,100);
        
        batch.SetColor(0.0f, 1.0f, 0.0f);
        font.Print(10,10,"FPS: %d", (int)(device.GetFPS()));
        font.Print(10,20,"Bump: %f", bump);
        batch.Render();
        
        device.Flip();
    }

    shadowManager.Cleanup();
    scene.Clear();
    font.Release();
    batch3D.Release();
    batch.Release();
    device.Close();



    return 0;
}
