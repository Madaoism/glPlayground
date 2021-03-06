#include "./Asset.h"
#include "../utils/Logger.h"

void Asset::addModel(const std::string& key, Model* model, bool addAsChild)
{
  if (!model) 
  {
    Log.print<Severity::warning>("Trying to add a null model to asset!");
    return;
  }

  _mModels[key] = model;
  if (addAsChild && !model->getParent()) 
  {
    addChild(model);
  }
}

Model* Asset::getModel(const std::string& key) const 
{
  auto it = _mModels.find(key);
  if (it != _mModels.end()) {
    return (*it).second;
  }
  return nullptr;
}

void Asset::copyTo(Cloneable* cloned) const
{
  Node::copyTo(cloned);
  Asset* clonedAsset = dynamic_cast<Asset*>(cloned);
  if (!clonedAsset)
  {
    Log.print < Severity::warning>("Failed to cast to Asset in clone");
    return;
  }

  for (auto pair : _mModels)
  {
    std::string key = pair.first;
    Model* model = pair.second;

    auto findModel = breadthFirstSearch(model);
    if (findModel.size() > 0)
    {
      Model* clonedModel = dynamic_cast<Model*>(clonedAsset->getDescendentByIndices(findModel));
      if (clonedModel)
        clonedAsset->_mModels[key] = clonedModel;
      else
        Log.print<Severity::warning>("Failed to cast found Model during Asset clone");
    }
    else
    {
      clonedAsset->_mModels[key] = model->clone();
    }
  }
}

Asset* Asset::clone() const
{
  Asset* a = new Asset();
  copyTo(a);
  return a;
}


void Asset::removeModel(const std::string& name)
{
  _mModels.erase(name);
}

std::map<std::string, Model*> Asset::getAllModels() const
{
  return _mModels;
}

void Asset::update(float deltaT)
{
  if (isAnimationStarted)
  {
    currentAnimationMs += deltaT;
  }

  Node::update(deltaT);
}


void Asset::draw(const glm::mat4& PV)
{

  std::map<const ShaderProgram*, Material*> uniqueMats;

  for (Material*& mat : allMaterials)
  {
    const ShaderProgram* p = mat->getProgram();
    if (uniqueMats.find(p) == uniqueMats.end())
    {
      uniqueMats[p] = mat;
    }
  }

  if (skeleton) {
    if (isAnimationStarted && currentAnimationIdx >= 0)
    {
      Animation* anim = skeleton->getAnimation(currentAnimationIdx);
      std::vector<glm::mat4> boneMatrices = skeleton->calcBoneMatrices(currentAnimationIdx, currentAnimationMs);

      for (auto it : uniqueMats)
      {
        it.second->setBoneMatrices(boneMatrices);
        it.second->setUseBoneTransform(true);
      }
    }
    else
    {
      const std::vector<glm::mat4>& boneMatrices = skeleton->getBindPoseMatrices();
      for (auto it : uniqueMats)
      {
        it.second->setBoneMatrices(boneMatrices);
        it.second->setUseBoneTransform(true);
      }
    }
  }

  Node::draw(PV);

  for (auto it : uniqueMats)
  {
    it.second->setUseBoneTransform(false);
  }
}