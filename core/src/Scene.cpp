
#include "pch.h"
#include "Scene.hpp"
#include "Mesh.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "glad/glad.h"
#include <algorithm>


static u32 IDS = 0;

SceneNode::SceneNode()
{
    _parent = nullptr;
    _id = IDS++;
    _sortKey = 0.0f;
    _dirty = true;
    _transformed = true;
    SetTransform( Vec3( 0.0f, 0.0f, 0.0f ), Vec3( 0.0f, 0.0f, 0.0f ), Vec3( 1.0f, 1.0f, 1.0f ) );
}

SceneNode::SceneNode(const Vec3 &trans, const Vec3 &scale) 
{
    _parent = nullptr;
    _id = IDS++;
    _sortKey = 0.0f;
    _dirty = true;
    _transformed = true;
    SetTransform( trans, Vec3( 0.0f, 0.0f, 0.0f ), scale );
}

SceneNode::SceneNode(const Vec3 &trans, const Vec3 &rot, const Vec3 &scale) 
{
    _parent = nullptr;
    _id = IDS++;
    _sortKey = 0.0f;
    _dirty = true;
    _transformed = true;
    SetTransform( trans, rot, scale );
}

SceneNode::~SceneNode()
{
   RemoveAllChildren();
}

void SceneNode::AddChild(SceneNode* child) {
    if (!child || child == this) return;
    
    // Remover do pai anterior
    if (child->_parent) {
        child->RemoveFromParent();
    }
    
    child->_parent = this;
    _children.push_back(child);
    child->markDirty();
}

void SceneNode::RemoveChild(SceneNode* child) 
{
    if (!child) return;
    
    auto it = std::find(_children.begin(), _children.end(), child);
    if (it != _children.end()) 
    {
        (*it)->_parent = nullptr;
        _children.erase(it);
        delete child;
    }
}

void SceneNode::RemoveFromParent() 
{
    if (_parent) 
    {
        _parent->RemoveChild(this);
    }
}

void SceneNode::RemoveAllChildren() 
{
    for (auto child : _children) 
    {
        child->_parent = nullptr;
    }
    _children.clear();
}

SceneNode* SceneNode::FindChild(const std::string& name) 
{
    for (auto child : _children) 
    {
        if (child->GetName() == name) 
        {
            return child;
        }
    }
    return nullptr;
}

SceneNode* SceneNode::FindChildById(u32 id) 
{
    for (auto child : _children) 
    {
        if (child->_id == id) 
        {
            return child;
        }
    }
    return nullptr;
}

void SceneNode::GetTransform( Vec3 &trans, Vec3 &rot, Vec3 &scale ) const
{
	if( _dirty ) Scene::Instance().UpdateNodes();
	
	_relTrans.decompose( trans, rot, scale );
	rot.x = radToDeg( rot.x );
	rot.y = radToDeg( rot.y );
	rot.z = radToDeg( rot.z );
}


void SceneNode::SetTransform( Vec3 trans, Vec3 rot, Vec3 scale )
{
	 
	if( _type == SceneNodeTypes::Joint )
	{
		//((JointNode *)this)->_parentModel->_skinningDirty = true;
	}
	
	_relTrans = Mat4::Scale( scale.x, scale.y, scale.z );
	_relTrans.rotate( degToRad( rot.x ), degToRad( rot.y ), degToRad( rot.z ) );
	_relTrans.translate( trans.x, trans.y, trans.z );
	
	markDirty();
}


void SceneNode::SetTransform( const Mat4 &mat )
{
	// Hack to avoid making setTransform virtual
	if( _type == SceneNodeTypes::Joint )
	{
		//((JointNode *)this)->_parentModel->_skinningDirty = true;
	}
	
	_relTrans = mat;
	
	markDirty();
}


void SceneNode::GetTransMatrices( const float **relMat, const float **absMat ) const
{
	if( relMat != 0x0 )
	{
		if( _dirty ) Scene::Instance().UpdateNodes();
		*relMat = &_relTrans.x[0];
	}
	
	if( absMat != 0x0 )
	{
		if( _dirty ) Scene::Instance().UpdateNodes();
		*absMat = &_absTrans.x[0];
	}
}


void SceneNode::markChildrenDirty()
{	
	for( std::vector< SceneNode * >::iterator itr = _children.begin(),
	     end = _children.end(); itr != end; ++itr )
	{
		if( !(*itr)->_dirty )
		{	
			(*itr)->_dirty = true;
			(*itr)->_transformed = true;
			(*itr)->markChildrenDirty();
		}
	}
}


void SceneNode::Update(float dt) {}

void SceneNode::Render(Shader* shader) {}

void SceneNode::RenderDepth(Shader * shader)
{
}




void SceneNode::updateTree()
{
	if( !_dirty ) return;
	
	// Calculate absolute matrix
	if( _parent != 0x0 )
		Mat4::fastMult43( _absTrans, _parent->_absTrans, _relTrans );
	else
		_absTrans = _relTrans;


//	onPostUpdate();

	_dirty = false;

	// Visit children
	for( u32 i = 0, s = (u32)_children.size(); i < s; ++i )
	{
		_children[i]->updateTree();
	}	

//	onFinishedUpdate();
}


bool SceneNode::CheckIntersection( const Vec3 &/*rayOrig*/, const Vec3 &/*rayDir*/, Vec3 &/*intsPos*/ ) const
{
	return false;
}


void SceneNode::MarkForDestroy() 
{
    Scene::Instance().RemoveNode(this);
}

 

// Position
void SceneNode::SetPosition(const Vec3& pos) 
{
    Vec3 currentRot, currentScale;
    GetTransform(_cachedPosition, currentRot, currentScale);
    SetTransform(pos, currentRot, currentScale);
}

Vec3 SceneNode::GetPosition() const 
{
    if (!_cacheValid) 
    {
        GetTransform(_cachedPosition, _cachedRotation, _cachedScale);
        _cacheValid = true;
    }
    return _cachedPosition;
}

Vec3 SceneNode::GetWorldPosition() const 
{
    if (_dirty) Scene::Instance().UpdateNodes();
    return Vec3(_absTrans.x[12], _absTrans.x[13], _absTrans.x[14]);
}

void SceneNode::Translate(const Vec3& delta) 
{
    SetPosition(GetPosition() + delta);
}

// Rotation
void SceneNode::SetRotation(const Vec3& rot) 
{
    Vec3 currentPos, currentScale;
    GetTransform(currentPos, _cachedRotation, currentScale);
    SetTransform(currentPos, rot, currentScale);
}

Vec3 SceneNode::GetRotation() const 
{
    if (!_cacheValid) {
        GetTransform(_cachedPosition, _cachedRotation, _cachedScale);
        _cacheValid = true;
    }
    return _cachedRotation;
}

void SceneNode::Rotate(const Vec3& delta) 
{
    SetRotation(GetRotation() + delta);
}

void SceneNode::RotateAround(const Vec3& point, const Vec3& axis, float angle) 
{
    Vec3 pos = GetWorldPosition();
    Vec3 dir = pos - point;
    
    Quaternion q = Quaternion::FromAxisAngle(axis, angle);//degToRad(angle));
 

    // Criar matriz de rotação
    Mat4 rotMat = Mat4(q);
    Vec3 newDir = Mat4::Transform(rotMat, dir); 
    
    SetWorldPosition(point + newDir);
    Rotate(axis * angle);
}

// Scale
void SceneNode::SetScale(const Vec3& scale) 
{
    Vec3 currentPos, currentRot;
    GetTransform(currentPos, currentRot, _cachedScale);
    SetTransform(currentPos, currentRot, scale);
}

Vec3 SceneNode::GetScale() const 
{
    if (!_cacheValid) 
    {
        GetTransform(_cachedPosition, _cachedRotation, _cachedScale);
        _cacheValid = true;
    }
    return _cachedScale;
}

void SceneNode::Scale(const Vec3& factor) 
{
    Vec3 currentScale = GetScale();
    SetScale(Vec3(currentScale.x * factor.x, 
                  currentScale.y * factor.y, 
                  currentScale.z * factor.z));
}

// Directions
Vec3 SceneNode::GetForward() const 
{
    if (_dirty) Scene::Instance().UpdateNodes();

    Vec3 dir = Mat4::Transform(_absTrans, Vec3(0, 0, -1));
    return Vec3::Normalize(dir);

}

Vec3 SceneNode::GetRight() const 
{
    if (_dirty) Scene::Instance().UpdateNodes();

    Vec3 dir = Mat4::Transform(_absTrans, Vec3(1, 0, 0));
    return Vec3::Normalize(dir);
}

Vec3 SceneNode::GetUp() const 
{
    if (_dirty) Scene::Instance().UpdateNodes();

    Vec3 dir = Mat4::Transform(_absTrans, Vec3(0, 1, 0));
    return Vec3::Normalize(dir);
}

// Look At
void SceneNode::LookAt(const Vec3& target, const Vec3& up) 
{
    // Vec3 pos = GetWorldPosition();
    // Vec3 forward = (target - pos).normalize();
    // Vec3 right = forward.cross(up).normalize();
    // Vec3 newUp = right.cross(forward).normalize();
    
    // Mat4 lookMat;
    // lookMat.x[0] = right.x;    lookMat.x[1] = right.y;    lookMat.x[2] = right.z;
    // lookMat.x[4] = newUp.x;    lookMat.x[5] = newUp.y;    lookMat.x[6] = newUp.z;
    // lookMat.x[8] = -forward.x; lookMat.x[9] = -forward.y; lookMat.x[10] = -forward.z;

    // Quaternion q = lookMat.;
    
    
    // Vec3 euler = lookMat.toEulerAngles();
    // SetRotation(Vec3(radToDeg(euler.x), radToDeg(euler.y), radToDeg(euler.z)));
}

void SceneNode::LookAt(const SceneNode* target, const Vec3& up) 
{
    if (target) 
    {
        LookAt(target->GetWorldPosition(), up);
    }
}

// World Space
void SceneNode::SetWorldPosition(const Vec3& worldPos) 
{
    if (_parent) 
    {
        Vec3 localPos = _parent->WorldToLocal(worldPos);
        SetPosition(localPos);
    } else 
    {
        SetPosition(worldPos);
    }
}

Vec3 SceneNode::WorldToLocal(const Vec3& worldPos) const 
{
    if (_dirty) Scene::Instance().UpdateNodes();
    Mat4 invMat = _absTrans.inverted();
    return  Mat4::Transform(invMat, worldPos);
     
}

Vec3 SceneNode::LocalToWorld(const Vec3& localPos) const 
{
    if (_dirty) Scene::Instance().UpdateNodes();
    return  Mat4::Transform(_absTrans, localPos);
    
}


void SceneNode::SetPositionSmooth(const Vec3& target, float speed) 
{
    Vec3 current = GetPosition();
    Vec3 newPos = current + (target - current) * speed;
    SetPosition(newPos);
}

// Override do markDirty para invalidar cache
void SceneNode::markDirty() 
{
    _dirty = true;
    _transformed = true;
    invalidateCache();
    updateTree();
    
    SceneNode *node = _parent;
    while (node != nullptr) 
    {
        node->_dirty = true;
        node->invalidateCache();
        node = node->_parent;
    }
    
    markChildrenDirty();
}

// void SceneNode::markDirty()
// {
// 	_dirty = true;
// 	_transformed = true;
	
// 	SceneNode *node = _parent;
// 	while( node != 0x0 )
// 	{
// 		node->_dirty = true;
// 		node = node->_parent;
// 	}

// 	markChildrenDirty();
// }
//*****************************************************************************
// Model
//*****************************************************************************

Material *Model::AddMaterial() 
{
    Material *mat = new Material();
    _materials.push_back(mat);
    return mat;
}

void Model::SetMaterial(u32 index, Material *mat) 
{
    if (index >= _materials.size())
        return;
    if (_materials[index])
        delete _materials[index];
    _materials[index] = mat;
}

void Model::SetTexture(u32 index, u32 layer, Texture* texture) 
{
    if (index >= _materials.size())
        return;
    
    _materials[index]->SetTexture(layer, texture);
}


Model::Model(SceneNode *parent) 
{
    _parent = parent;
    _type = SceneNodeTypes::Model;
}

Model::~Model() 
{
    _meshes.clear();
    for (auto mat : _materials)
    {
        delete mat;
    }
    _materials.clear();
}

void Model::AddMesh(Mesh *mesh) 
{
    if (!mesh)  return;
    _meshes.push_back(mesh); 
}

void Model::Update(float dt) {}

void Model::Render(Shader* shader) 
{
    if (!shader) return;
    shader->SetMatrix4("model", &_absTrans.x[0]);
    for (auto mesh : _meshes)
    {
        if (GetMaterialCount()) 
        {
            if (mesh->m_material <= _materials.size()) 
            {
                Material *mat = _materials[mesh->m_material];
                mat->Bind();
            }
        } else 
        {
            Material *mat = Scene::Instance().GetDefaultMaterial();
            mat->Bind();

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, 0);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, 0);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, 0);

            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, 0);
            
            
        }

        mesh->Render();
    }
}




//**********************************************************************************************
//Scene
//**********************************************************************************************

void Scene::AddNode(SceneNode *node) 
{
    if (!node) return;
    _nodes_to_add.push_back(node);
}

void Scene::RemoveNode(SceneNode *node) 
{
    if (!node) return;
     RemoveNode(node->_id);
}

void Scene::RemoveNode(u32 nodeId) 
{
    _nodes_to_remove.push_back(nodeId);
}

void Scene::RemoveNodeByName(const std::string& name) 
{
    auto it = std::find_if(_nodes.begin(), _nodes.end(), 
        [&name](SceneNode* node) { return node->GetName() == name; });
    
    if (it != _nodes.end()) 
    {
        RemoveNode((*it)->_id);
    }
}

SceneNode* Scene::FindNodeById(u32 id) 
{
    auto it = std::find_if(_nodes.begin(), _nodes.end(),
        [&id](SceneNode* node) { return node->_id == id; });
    return (it != _nodes.end()) ? *it : nullptr;
}

SceneNode* Scene::FindNodeByName(const std::string& name) 
{
    auto it = std::find_if(_nodes.begin(), _nodes.end(),
        [&name](SceneNode* node) { return node->GetName() == name; });
    return (it != _nodes.end()) ? *it : nullptr;
}

Model* Scene::CreateModel(const std::string& name) 
{ 
    Model *model = new Model();
    model->SetName(name);
    AddNode(model);
    return model;
}

SceneNode* Scene::CreateNode(const std::string& name)
{
    SceneNode *node = new SceneNode();
    node->SetName(name);
    AddNode(node);
    return node;
}

void Scene::Clear() 
{
    for (auto node : _nodes)
    {
        delete node;
    }
    for (auto node : _nodes_to_add)
    {
        delete node;
    }

    _nodes.clear();
    _nodes_to_add.clear();
    _nodes_to_remove.clear();
}

void Scene::Update(float dt) 
{
    for (auto node : _nodes_to_add)
    {
        _nodes.push_back(node);
    }
    _nodes_to_add.clear();
  
    for (auto nodeId : _nodes_to_remove)
    {
        auto it = std::find_if(_nodes.begin(), _nodes.end(),
            [&nodeId](SceneNode* node) { return node->_id == nodeId; });
        if (it != _nodes.end())
        {
            delete *it;
            _nodes.erase(it);
        }
    }
    _nodes_to_remove.clear();

    for (auto node : _nodes)
    {
        node->Update(dt);
    }
}


void Model::RenderDepth(Shader* shader) 
{
    if (!shader) return;
    shader->SetMatrix4("model", &_absTrans.x[0]);
    for (auto mesh : _meshes)
    {
        if (mesh->CastsShadows())
        {
            mesh->Render();
        }
    }
}

void Scene::Render() 
{
    for (auto node : _nodes)
    {
        node->Render( m_defaultShader );
    }
}

void Scene::RenderDepth(Shader* shader) 
{
    glDisable(GL_CULL_FACE);
    //  glCullFace(GL_FRONT);
    for (auto node : _nodes)
    {
        node->RenderDepth( shader );
    }
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
}

Scene &Scene::Instance()
{
    static Scene instance;
    return instance;
}

Scene *Scene::InstancePtr() 
{
    return &Instance();
}

void Scene::UpdateNodes() 
{
    for (auto node : _nodes)
    {
        node->updateTree();
    }
}

Scene::Scene() 
{
    m_defaultMaterial = new Material();
    m_defaultMaterial->SetTexture(0, TextureManager::Instance().GetDefault());
    
}

Scene::~Scene() 
{
    
    delete m_defaultMaterial;
}
