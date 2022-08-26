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

#pragma once

#include <string>
#include <vector>

#include "cinder/Cinder.h"
#include "cinder/Vector.h"
#include "cinder/Quaternion.h"
#include "cinder/Matrix.h"

namespace sitara {
	namespace assimp {
		class AssimpNode;
		typedef std::shared_ptr< AssimpNode > AssimpNodeRef;

		// forward declare
		class AssimpMesh;
        typedef std::shared_ptr<AssimpMesh> AssimpMeshRef;


		class AssimpNode
		{
		public:
			AssimpNode();
			AssimpNode(const std::string& name);
			virtual ~AssimpNode() {};

			void setParent(AssimpNodeRef parent);
			AssimpNodeRef getParent() const;

			void addChild(AssimpNodeRef child);

			void setOrientation(const ci::quat& q);
			const ci::quat& getOrientation() const;

			void setPosition(const ci::vec3& pos);
			const ci::vec3& getPosition() const;

			void setScale(const ci::vec3& scale);
			const ci::vec3& getScale() const;

			void setInheritOrientation(bool inherit);
			bool getInheritOrientation() const;

			void setInheritScale(bool inherit);
			bool getInheritScale() const;

			void setName(const std::string& name);
			const std::string& getName() const;

			void setInitialState();
			void resetToInitialState();
			const ci::vec3& getInitialPosition() const;
			const ci::quat& getInitialOrientation() const;
			const ci::vec3& getInitialScale() const;

			const ci::quat& getDerivedOrientation() const;
			const ci::vec3& getDerivedPosition() const;
			const ci::vec3& getDerivedScale() const;

			const ci::mat4& getDerivedTransform() const;

			std::vector<AssimpMeshRef>& getMeshes();

			void requestUpdate();

		protected:
			/// Shared pointer to parent AssimpNode.
			AssimpNodeRef mParent;

			/// Shared pointer vector holding the children.
			std::vector< AssimpNodeRef > mChildren;

			/// Name of this AssimpNode.
			std::string mName;

			/// The orientation of the AssimpNode relative to its parent.
			ci::quat mOrientation;

			/// The position of the AssimpNode relative to its parent.
			ci::vec3 mPosition;

			/// Scaling factor applied to this AssimpNode.
			ci::vec3 mScale;

			/// Stores whether this AssimpNode inherits orientation from its parent.
			bool mInheritOrientation;

			mutable bool mNeedsUpdate;

			/// Stores whether this AssimpNode inherits scale from its parent
			bool mInheritScale;

			/** Cached combined orientation.
				This member is the orientation derived by combining the
				local transformations and those of it's parents.
				*/
			mutable ci::quat mDerivedOrientation;

			/** Cached combined position.
				This member is the position derived by combining the
				local transformations and those of it's parents.
				*/
			mutable ci::vec3 mDerivedPosition;

			/** Cached combined scale.
				This member is the position derived by combining the
				local transformations and those of it's parents.
				*/
			mutable ci::vec3 mDerivedScale;

			/// The position to use as a base for keyframe animation.
			ci::vec3 mInitialPosition;
			/// The orientation to use as a base for keyframe animation.
			ci::quat mInitialOrientation;
			/// The scale to use as a base for keyframe animation.
			ci::vec3 mInitialScale;

			/// Cached derived transform as a 4x4 matrix
			mutable ci::mat4 mDerivedTransform;
            std::vector<AssimpMeshRef> mMeshes;

			void update() const;
		};
	}
}

