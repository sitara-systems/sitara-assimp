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

namespace mndl {

Node::Node() :
	mScale( vec3(1) ),
	mInheritOrientation( true ),
	mInheritScale( true ),
	mNeedsUpdate( true )
{
}

Node::Node( const std::string &name ) :
	mName( name ),
	mScale( vec3(1) ),
	mInheritOrientation( true ),
	mInheritScale( true ),
	mNeedsUpdate( true )
{
}

void Node::setParent( NodeRef parent )
{
	mParent = parent;
	requestUpdate();
}

NodeRef Node::getParent() const
{
	return mParent;
}

void Node::addChild( NodeRef child )
{
	mChildren.push_back( child );
}

void Node::setOrientation( const ci::quat &q )
{
	mOrientation = q;
	mOrientation = glm::normalize(mOrientation);
	requestUpdate();
}

const quat &Node::getOrientation() const
{
	return mOrientation;
}

void Node::setPosition( const ci::vec3 &pos )
{
	mPosition = pos;
	requestUpdate();
}

const vec3& Node::getPosition() const
{
	return mPosition;
}

void Node::setScale( const vec3 &scale )
{
	mScale = scale;
	requestUpdate();
}

const vec3 &Node::getScale() const
{
	return mScale;
}

void Node::setInheritOrientation( bool inherit )
{
	mInheritOrientation = inherit;
	requestUpdate();
}

bool Node::getInheritOrientation() const
{
	return mInheritOrientation;
}

void Node::setInheritScale( bool inherit )
{
	mInheritScale = inherit;
	requestUpdate();
}

bool Node::getInheritScale() const
{
	return mInheritScale;
}

void Node::setName( const string &name )
{
	mName = name;
}

const string &Node::getName() const
{
	return mName;
}

void Node::setInitialState()
{
	mInitialPosition = mPosition;
	mInitialOrientation = mOrientation;
	mInitialScale = mScale;
}

void Node::resetToInitialState()
{
	mPosition = mInitialPosition;
	mOrientation = mInitialOrientation;
	mScale = mInitialScale;
	requestUpdate();
}

const vec3 &Node::getInitialPosition() const
{
	return mInitialPosition;
}

const quat &Node::getInitialOrientation() const
{
	return mInitialOrientation;
}

const vec3 &Node::getInitialScale() const
{
	return mInitialScale;
}

const quat &Node::getDerivedOrientation() const
{
	if ( mNeedsUpdate )
		update();
	return mDerivedOrientation;
}

const vec3 &Node::getDerivedPosition() const
{
	if ( mNeedsUpdate )
		update();
	return mDerivedPosition;
}

const vec3 &Node::getDerivedScale() const
{
	if ( mNeedsUpdate )
		update();
	return mDerivedScale;
}

const mat4 &Node::getDerivedTransform() const
{
	if ( mNeedsUpdate )
		update();

    mDerivedTransform = glm::scale(mDerivedScale);
    mDerivedTransform *= ci::mat4(mDerivedOrientation);
    mDerivedTransform *= glm::translate(mDerivedPosition);

    return mDerivedTransform;
}

void Node::update() const
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
        // root node, no parent
        mDerivedOrientation = getOrientation();
        mDerivedPosition = getPosition();
        mDerivedScale = getScale();
    }

	mNeedsUpdate = false;
}

void Node::requestUpdate()
{
	mNeedsUpdate = true;

	for ( vector< NodeRef >::iterator it = mChildren.begin();
			it != mChildren.end(); ++it )
	{
		(*it)->requestUpdate();
	}
}

} // namespace mndl
