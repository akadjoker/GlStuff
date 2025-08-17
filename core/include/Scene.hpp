
#pragma once


#include "Config.hpp"
#include "Texture.hpp"
#include "Device.hpp"
#include "glad/glad.h"
#include "Math.hpp"

#include <vector>
#include <string>
 

class Mesh;
class Scene;
class Model;
class Material;
class Shader;

struct SceneNodeTypes
{
	enum List
	{
		Undefined = 0,
		Group,
		Model,
		Mesh,
		Joint,
		Light,
		Camera,
		Emitter,
		Compute
	};
};


class CORE_PUBLIC SceneNode
{
public:
	SceneNode( );
    SceneNode(const Vec3 &trans,  const Vec3 &scale);
    SceneNode(const Vec3 &trans, const Vec3 &rot, const Vec3 &scale);
	virtual ~SceneNode();

	void GetTransform( Vec3 &trans, Vec3 &rot, Vec3 &scale ) const; 	
    void SetTransform( Vec3 trans, Vec3 rot, Vec3 scale );
	void SetTransform( const Mat4 &mat );
	void GetTransMatrices( const float **relMat, const float **absMat ) const;

    void SetName( const std::string &name ) { _name = name; }
	virtual bool CheckIntersection( const Vec3 &rayOrig, const Vec3 &rayDir, Vec3 &intsPos ) const;
    
	virtual void SetCustomInstData( const float *data, u32 count ) {}
    
	
    SceneNode *GetParent() const { return _parent; }
    
    void SetParent( SceneNode *parent ) { _parent = parent; }
    const std::string GetName() const { return _name; }
	
    std::vector< SceneNode * > &GetChildren() { return _children; }
	
    Mat4 &GetRelTrans() { return _relTrans; }
	
    Mat4 &GetAbsTrans() { return _absTrans; }
	
    BoundingBox &GetBBox() { return _bBox; }
	
    
    virtual void Update(float dt);
    virtual void Render(Shader* shader);
    virtual void RenderDepth(Shader* shader);
    
	
	void markDirty();
	void updateTree();

        // === POSITION ===
    void SetPosition(const Vec3& pos);
    void SetPosition(float x, float y, float z) { SetPosition(Vec3(x, y, z)); }
    Vec3 GetPosition() const;
    Vec3 GetWorldPosition() const;
    
    void Translate(const Vec3& delta);
    void Translate(float x, float y, float z) { Translate(Vec3(x, y, z)); }
    
    // === ROTATION ===
    void SetRotation(const Vec3& rot);
    void SetRotation(float x, float y, float z) { SetRotation(Vec3(x, y, z)); }
    Vec3 GetRotation() const;
    Vec3 GetWorldRotation() const;
    
    void Rotate(const Vec3& delta);
    void Rotate(float x, float y, float z) { Rotate(Vec3(x, y, z)); }
    void RotateAround(const Vec3& point, const Vec3& axis, float angle);
    
    // === SCALE ===
    void SetScale(const Vec3& scale);
    void SetScale(float scale) { SetScale(Vec3(scale, scale, scale)); }
    void SetScale(float x, float y, float z) { SetScale(Vec3(x, y, z)); }
    Vec3 GetScale() const;
    
    void Scale(const Vec3& factor);
    void Scale(float factor) { Scale(Vec3(factor, factor, factor)); }
    
    // === DIRECTIONS ===
    Vec3 GetForward() const;
    Vec3 GetRight() const;
    Vec3 GetUp() const;
    
    // === LOOK AT ===
    void LookAt(const Vec3& target, const Vec3& up = Vec3(0, 1, 0));
    void LookAt(const SceneNode* target, const Vec3& up = Vec3(0, 1, 0));
    
    // === INTERPOLATION ===
    void SetPositionSmooth(const Vec3& target, float speed);
    void SetRotationSmooth(const Vec3& target, float speed);
    
    // === WORLD SPACE OPERATIONS ===
    void SetWorldPosition(const Vec3& worldPos);
    void SetWorldRotation(const Vec3& worldRot);
    Vec3 WorldToLocal(const Vec3& worldPos) const;
    Vec3 LocalToWorld(const Vec3& localPos) const;
    
	
    void AddChild(SceneNode* child);
    void RemoveChild(SceneNode* child);
    void RemoveFromParent();
    void RemoveAllChildren();


    void MarkForDestroy();

 
    SceneNode* FindChild(const std::string& name);
    SceneNode* FindChildById(u32 id);
    
    
    protected:
	Mat4                    _relTrans, _absTrans;  // Transformation matrices
	std::vector< SceneNode * >  _children;  // Child nodes
    u32                         _id;
    
    
	std::string                 _name;
	std::string                 _attachment;  // User defined data
	BoundingBox                 _bBox;  // AABB in world space
	SceneNode                   *_parent;  // Parent node
    
	int                         _type;
    
	float                       _sortKey;
	bool                        _dirty;  // Does the node need to be updated?
	bool                        _transformed;
    
    mutable Vec3 _cachedPosition;
    mutable Vec3 _cachedRotation;
    mutable Vec3 _cachedScale;
    mutable bool _cacheValid = false;
    
    void invalidateCache() { _cacheValid = false; }
    void markChildrenDirty();
	friend class Scene;
};


class CORE_PUBLIC Scene 
{
public:

    void AddNode( SceneNode *node );
    void RemoveNode( SceneNode *node );
    void RemoveNode(u32 nodeId);
    void RemoveNodeByName(const std::string& name);
    SceneNode* FindNodeById(u32 id);
    SceneNode* FindNodeByName(const std::string& name);

    Model* CreateModel(const std::string& name="Model");
    SceneNode* CreateNode(const std::string& name="Node");

    void Clear();
    
    void Update(float dt);
    void Render();
    void RenderDepth(Shader* shader);

    void SetShader( Shader *shader ) { m_defaultShader = shader; }

    static Scene& Instance();
    static Scene* InstancePtr();

    void UpdateNodes(  );

    Material* GetDefaultMaterial() { return m_defaultMaterial; }
    
    private:


    Shader *m_defaultShader;
    Material *m_defaultMaterial;


    std::vector< SceneNode * > _nodes;    
    std::vector< SceneNode * > _nodes_to_add;  
     std::vector<u32> _nodes_to_remove;
 


    Scene();
    ~Scene();

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) = delete;
    Scene& operator=(Scene&&) = delete;
};

class CORE_PUBLIC Model : public SceneNode
{
public:
    Model(SceneNode *parent = nullptr);
    ~Model();

    void AddMesh(Mesh* mesh);

    void Update(float dt);
    void Render(Shader* shader);
    void RenderDepth(Shader* shader);

    Material* AddMaterial();

    void SetMaterial(u32 index, Material *mat);
    void SetTexture(u32 index,u32 layer, Texture* texture);

    u32 GetMeshCount() const { return (u32)_meshes.size(); }
    u32 GetMaterialCount() const { return (u32)_materials.size(); }

private:
    std::vector<Mesh*> _meshes;
    std::vector<Material*> _materials;
};