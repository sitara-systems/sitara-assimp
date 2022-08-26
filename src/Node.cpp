/*
 Copyright (C) 2011-2012 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published
 by the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "Node.h"

using namespace ci;
using namespace std;
using namespace sitara::assimp;

AssimpNode::AssimpNode() :
	mScale( vec3(1) ),
	mInheritOrientation( true ),
	mInheritScale( true ),
	mNeedsUpdate( true )
{
}

AssimpNode::AssimpNode( const std::string &name ) :
	mName( name ),
	mScale( vec3(1) ),
	mInheritOrientation( true ),
	mInheritScale( true ),
	mNeedsUpdate( true )
{
}

void AssimpNode::setParent( AssimpNodeRef parent )
{
	mParent = parent;
	requestUpdate();
}

AssimpNodeRef AssimpNode::getParent() const
{
	return mParent;
}

void AssimpNode::addChild( AssimpNodeRef child )
{
	mChildren.push_back( child );
}

void AssimpNode::setOrientation( const ci::quat &q )
{
	mOrientation = q;
	mOrientation = glm::normalize(mOrientation);
	requestUpdate();
}

const quat &AssimpNode::getOrientation() const
{
	return mOrientation;
}

void AssimpNode::setPosition( const ci::vec3 &pos )
{
	mPosition = pos;
	requestUpdate();
}

const vec3& AssimpNode::getPosition() const
{
	return mPosition;
}

void AssimpNode::setScale( const vec3 &scale )
{
	mScale = scale;
	requestUpdate();
}

const vec3 &AssimpNode::getScale() const
{
	return mScale;
}

void AssimpNode::setInheritOrientation( bool inherit )
{
	mInheritOrientation = inherit;
	requestUpdate();
}

bool AssimpNode::getInheritOrientation() const
{
	return mInheritOrientation;
}

void AssimpNode::setInheritScale( bool inherit )
{
	mInheritScale = inherit;
	requestUpdate();
}

bool AssimpNode::getInheritScale() const
{
	return mInheritScale;
}

void AssimpNode::setName( const string &name )
{
	mName = name;
}

const string &AssimpNode::getName() const
{
	return mName;
}

void AssimpNode::setInitialState()
{
	mInitialPosition = mPosition;
	mInitialOrientation = mOrientation;
	mInitialScale = mScale;
}

void AssimpNode::resetToInitialState()
{
	mPosition = mInitialPosition;
	mOrientation = mInitialOrientation;
	mScale = mInitialScale;
	requestUpdate();
}

const vec3 &AssimpNode::getInitialPosition() const
{
	return mInitialPosition;
}

const quat &AssimpNode::getInitialOrientation() const
{
	return mInitialOrientation;
}

const vec3 &AssimpNode::getInitialScale() const
{
	return mInitialScale;
}

const quat &AssimpNode::getDerivedOrientation() const
{
	if ( mNeedsUpdate )
		update();
	return mDerivedOrientation;
}

const vec3 &AssimpNode::getDerivedPosition() const
{
	if ( mNeedsUpdate )
		update();
	return mDerivedPosition;
}

const vec3 &AssimpNode::getDerivedScale() const
{
	if ( mNeedsUpdate )
		update();
	return mDerivedScale;
}

const mat4 &AssimpNode::getDerivedTransform() const
{
	if ( mNeedsUpdate )
		update();

    mDerivedTransform = glm::scale(mDerivedScale);
    mDerivedTransform *= ci::mat4(mDerivedOrientation);
    mDerivedTransform *= glm::translate(mDerivedPosition);

    return mDerivedTransform;
}

std::vector<AssimpMeshRef>& AssimpNode::getMeshes() {
    return mMeshes;
}

void AssimpNode::update() const
{
    if ( mParent )
    {
        // update orientation
        const quat &parentOrientation = mParent->getDerivedOrientation();
        if ( mInheritOrientation )
        {
            // Combine orientation with that of parent
            mDerivedOrientation = getOrientation() * parentOrientation;
        }
        else
        {
            mDerivedOrientation = getOrientation();
        }

        // update scale
        const vec3 &parentScale = mParent->getDerivedScale();
        if ( mInheritScale )
        {
            mDerivedScale = parentScale * getScale();
        }
        else
        {
            mDerivedScale = getScale();
        }

		// change position vector based on parent's orientation & scale
        mDerivedPosition = ( parentScale * getPosition() ) * parentOrientation;

        // add altered position vector to parent's
        mDerivedPosition += mParent->getDerivedPosition();
    }
    else
    {
        // root AssimpNode, no parent
        mDerivedOrientation = getOrientation();
        mDerivedPosition = getPosition();
        mDerivedScale = getScale();
    }

	mNeedsUpdate = false;
}

void AssimpNode::requestUpdate()
{
	mNeedsUpdate = true;

	for ( vector< AssimpNodeRef >::iterator it = mChildren.begin();
			it != mChildren.end(); ++it )
	{
		(*it)->requestUpdate();
	}
}