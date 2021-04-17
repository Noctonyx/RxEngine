//
// Copyright (c) 2020 - Shane Hyde (shane@noctonyx.com)
//

#include "Delegates.hpp"

namespace RxEngine
{
    __int64 DelegateHandle::CURRENT_ID = 0;

    __int64 DelegateHandle::GetNewID()
    {
        __int64 output = DelegateHandle::CURRENT_ID++;
        if (DelegateHandle::CURRENT_ID == 0) {
            DelegateHandle::CURRENT_ID = 1;
        }
        return output;
    }
}