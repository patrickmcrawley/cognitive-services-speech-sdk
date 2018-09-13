//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.md file in the project root for full license information.
//
// speechapi_c_keyword_recognition_model.h: Public API declarations for KeywordRecognitionModel related C methods and typedefs
//

#pragma once
#include <speechapi_c_common.h>


SPXAPI_(bool) keyword_recognition_model_handle_is_valid(SPXKEYWORDHANDLE hkeyword);
SPXAPI keyword_recognition_model_handle_release(SPXKEYWORDHANDLE hkeyword);

SPXAPI keyword_recognition_model_create_from_file(const char* fileName, SPXKEYWORDHANDLE* phkwmodel);
