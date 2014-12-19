
#include <stdafx.h>
#include <SLSkeleton.h>
#include <SLScene.h>
#include <SLAnimationState.h>


//-----------------------------------------------------------------------------
/*! @todo document
*/
SLSkeleton::SLSkeleton()
: _minOS(-1, -1, -1), _maxOS(1, 1, 1), _minMaxOutOfDate(true)
{
    SLScene::current->animManager().addSkeleton(this);
}

//-----------------------------------------------------------------------------
/*! @todo document
*/
SLSkeleton::~SLSkeleton()
{
    delete _root;

    map<SLstring, SLAnimation*>::iterator it1;
    for (it1 = _animations.begin(); it1 != _animations.end(); it1++)
        delete it1->second;
    
    map<SLstring, SLAnimationState*>::iterator it2;
    for (it2 = _animationStates.begin(); it2 != _animationStates.end(); it2++)
        delete it2->second;
}

//-----------------------------------------------------------------------------
/*! @todo document
*/
SLJoint* SLSkeleton::createJoint(SLuint handle)
{
    ostringstream oss;
    oss << "Joint " << handle;
    return createJoint(oss.str(), handle);
}

//-----------------------------------------------------------------------------
/*! @todo document
*/
SLJoint* SLSkeleton::createJoint(const SLstring& name, SLuint handle)
{
    SLJoint* result = new SLJoint(name, handle, this);
    
    assert((handle >= _jointList.size() || (handle < _jointList.size() && _jointList[handle] == NULL)) && "Trying to create a joint with an already existing handle.");

    if (_jointList.size() <= handle)
        _jointList.resize(handle+1);
    
    _jointList[handle] = result;
    return result;
}


//-----------------------------------------------------------------------------
/*! @todo document
*/
SLAnimationState* SLSkeleton::getAnimationState(const SLstring& name)
{
    if (_animationStates.find(name) != _animationStates.end())
        return _animationStates[name];

    else if (_animations.find(name) != _animations.end())
    {
        _animationStates[name] = new SLAnimationState(_animations[name]);
        return _animationStates[name];
    }

    return NULL;
}

//-----------------------------------------------------------------------------
/*! @todo document
*/
SLJoint* SLSkeleton::getJoint(SLuint handle)
{
    assert(handle < _jointList.size() && "Index out of bounds");
    return _jointList[handle];
}

//-----------------------------------------------------------------------------
/*! @todo document
*/
SLJoint* SLSkeleton::getJoint(const SLstring& name)
{
    if (!_root) return NULL;

    SLJoint* result = _root->find<SLJoint>(name);
    return result;
}

//-----------------------------------------------------------------------------
/*! @todo document
*/
void SLSkeleton::getJointWorldMatrices(SLMat4f* jointWM)
{
    // @todo this is asking for a crash...
    for (SLint i = 0; i < _jointList.size(); i++)
    {
        jointWM[i] = _jointList[i]->updateAndGetWM() * _jointList[i]->offsetMat();
    }
}

//-----------------------------------------------------------------------------
/*! @todo document
*/
void SLSkeleton::root(SLJoint* joint)
{
    if (_root)

    _root = joint;
}

//-----------------------------------------------------------------------------
/*! @todo document
*/
SLAnimation* SLSkeleton::createAnimation(const SLstring& name, SLfloat duration)
{
    assert(_animations.find(name) == _animations.end() && "animation with same name already exists!");
    SLAnimation* anim = new SLAnimation(name, duration);
    _animations[name] = anim;
    return anim;
}

//-----------------------------------------------------------------------------
/*! @todo document
*/
void SLSkeleton::reset()
{
    // mark the skeleton as changed
    changed(true);
    // update all joints
    for (SLint i = 0; i < _jointList.size(); i++)
        _jointList[i]->resetToInitialState();
}

//-----------------------------------------------------------------------------
/*! @todo document
*/
void SLSkeleton::updateAnimations()
{
    SLScene* scene = SLScene::current;

    /// @todo IMPORTANT: don't do this if we don't have any enabled animations, current workaround won't allow for blending!
    //reset();

    // first advance time on all animations
    _changed = false;
    map<SLstring, SLAnimationState*>::iterator it;
    for (it = _animationStates.begin(); it != _animationStates.end(); it++)
    {
        SLAnimationState* state = it->second;
        if (state->enabled())
        {
            state->advanceTime(scene->elapsedTimeSec());
            // mark skeleton as changed if a state is different
            if (state->changed())
                _changed = true;
        }
    }

    // return if nothing changed
    if (!_changed)
        return;

    // reset the skeleton and apply all enabled animations
    reset();

    for (it = _animationStates.begin(); it != _animationStates.end(); it++)
    {
        SLAnimationState* state = it->second;
        if (state->enabled())
        {
            state->parentAnimation()->apply(this, state->localTime(), state->weight());
            state->changed(false); // remove changed dirty flag from the state again
        }
    }

    _minMaxOutOfDate = true;
}


//-----------------------------------------------------------------------------
/*! @todo document
*/
const SLVec3f& SLSkeleton::minOS()
{
    if (_minMaxOutOfDate)
        updateMinMax();
    
    return _minOS;
}

//-----------------------------------------------------------------------------
/*! @todo document
*/
const SLVec3f& SLSkeleton::maxOS()
{
    if (_minMaxOutOfDate)
        updateMinMax();

    return _maxOS;
}

//-----------------------------------------------------------------------------
/*! @todo document
*/
void SLSkeleton::updateMinMax()
{    
    // recalculate the new min and max os based on bone radius
    SLbool firstSet = false;
    for (SLint i = 0; i < _jointList.size(); i++)
    {
        SLfloat r = _jointList[i]->radius();
        // ignore joints with a zero radius
        if (r == 0.0f)
            continue;

        SLVec3f jointPos = _jointList[i]->updateAndGetWM().translation();
        SLVec3f curMin = jointPos - SLVec3f(r, r, r);
        SLVec3f curMax = jointPos + SLVec3f(r, r, r);
        if (!firstSet)
        {
            _minOS = curMin;
            _maxOS = curMax;
            firstSet = true;
        }
        else
        {
            _minOS.x = min(_minOS.x, curMin.x);
            _minOS.y = min(_minOS.y, curMin.y);
            _minOS.z = min(_minOS.z, curMin.z);

            _maxOS.x = max(_maxOS.x, curMax.x);
            _maxOS.y = max(_maxOS.y, curMax.y);
            _maxOS.z = max(_maxOS.z, curMax.z);
        }
    }
    _minMaxOutOfDate = false;
}