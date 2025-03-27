#include "lpn.h"
#include <cassert>
#include <cryptoTools/Common/Defines.h>
#include <vector>
#include "context.h"
#include "joinData.h"

Matrix khprf(const std::vector<block> &r, const std::vector<block> &J)
{
    Matrix result(r.size(), J.size());
    for (int i = 0; i < r.size(); i++) {
        for (int j = 0; j < J.size(); j++) {
            result(i, j) = r[i] * J[j];
        }
    }
    return result;
}
