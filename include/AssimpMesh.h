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

#include <vector>
#include <string>

#include "assimp/mesh.h"

#include "cinder/Cinder.h"
#include "cinder/TriMesh.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Batch.h"

namespace sitara {
	namespace assimp {
		class AssimpMesh;
		typedef std::shared_ptr< AssimpMesh > AssimpMeshRef;

		struct Material {
			int mFace;
			ci::ColorAf mDiffuse;
			ci::ColorAf mSpecular;
			ci::ColorAf mAmbient;
			ci::ColorAf mEmission;

		};

		class AssimpMesh {
			public:
				const aiMesh *mAiMesh;

				ci::gl::Texture2dRef mTexture = nullptr;

				Material mMaterial;

				std::vector< uint32_t > mIndices;

				bool mTwoSided;

				std::vector<aiVector3D > mAnimatedPos;
				std::vector<aiVector3D > mAnimatedNorm;

				std::string mName;
				ci::TriMesh mCachedTriMesh;
				bool mValidCache;

		};
	}
}
