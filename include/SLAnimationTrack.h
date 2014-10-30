//#############################################################################
//  File:      SLAnimation.h
//  Author:    Marcus Hudritsch
//  Date:      July 2014
//  Codestyle: https://code.google.com/p/slproject/wiki/CodingStyleGuidelines
//  Copyright: 2002-2014 Marcus Hudritsch
//             This software is provide under the GNU General Public License
//             Please visit: http://opensource.org/licenses/GPL-3.0
//#############################################################################


#ifndef SLANIMATIONTRACK_H
#define SLANIMATIONTRACK_H

#include <stdafx.h>
#include <SLKeyframe.h>

class SLNode;
class SLAnimation;

// @todo order the keyframe sets based on their time value
// @todo provide an iterator over the keyframes

/** Virtual baseclass for specialized animation clips.
*/
class SLAnimationTrack
{
public:
    SLAnimationTrack(SLAnimation* parent, SLuint handle);
    ~SLAnimationTrack();

    SLKeyframe* createKeyframe(SLfloat time);   // create and add a new keyframe

    SLfloat         getKeyframesAtTime(SLfloat time, SLKeyframe** k1, SLKeyframe** k2) const;
    virtual void    calcInterpolatedKeyframe(SLfloat time, SLKeyframe* keyframe) const = 0; // we need a way to get an output value for a time we put in
	virtual void	apply(SLfloat time, SLfloat weight = 1.0f, SLfloat scale = 1.0f) = 0; // applies the animation clip to its target

protected:
    SLAnimation*            _parent;
    SLuint                  _handle;            //!< unique handle for this track inside its parent animation
    vector<SLKeyframe*>     _keyframeList;
	SLfloat                 _localTime;

    void keyframesChanged();
    virtual SLKeyframe* createKeyframeImpl(SLfloat time) = 0;

};


/** Specialized animation track for animating nodes
*/
class SLNodeAnimationTrack : public SLAnimationTrack
{
public:
    SLNodeAnimationTrack(SLAnimation* parent, SLuint handle);    

    SLTransformKeyframe* createNodeKeyframe(SLfloat time);

    virtual void calcInterpolatedKeyframe(SLfloat time, SLKeyframe* keyframe) const;
    // apply this track to a specified node
    virtual void apply(SLfloat time, SLfloat weight = 1.0f, SLfloat scale = 1.0f);
    virtual void applyToNode(SLNode* node, SLfloat time, SLfloat weight = 1.0f, SLfloat scale = 1.0f);

protected:
    SLNode*         _animationTarget;   //!< the default target for this track

    virtual SLKeyframe* createKeyframeImpl(SLfloat time);
};


#endif








