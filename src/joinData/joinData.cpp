#include "joinData.h"
#include <csignal>
#include <cstdint>
#include <vector>
#include <volePSI/Defines.h>

joinData::joinData(const PsiAnalyticsContext &context)
{
    volePSI::PRNG prng({ 5260, 1080 });
    switch (context.role) {
    case PA:
        features = Matrix(context.pa_elems, context.pa_features);
        prng.get(features.data(), context.pa_elems * context.pa_features);
        ids = std::vector<uint64_t>(context.pa_elems);
        prng.get(ids.data(), context.pa_elems);
        break;
    case PB:
        features = Matrix(context.pb_elems, context.pb_features);
        prng.get(features.data(), context.pb_elems * context.pb_features);
        ids = std::vector<uint64_t>(context.pb_elems);
        prng.get(ids.data(), context.pb_elems);
        break;
    }
}

// joinData::joinData(std::string config_file, std::string data_file)
// {

//     csv::CSVReader reader(data_file);
//     for (auto &row : reader) {
//         uint64_t id = std::atoi(row[0].get().c_str());
//         ids.push_back(id);
//         std::vector<feature> feature_row;
//         for (size_t index = 1; index < row.size(); index++) {
//             feature f(configs[index - 1].name, configs[index - 1].size);
//             switch (configs[index - 1].type) {
//             case INT:
//                 serialize(row[index].get<block>(), f.data, configs[index - 1].size);
//                 f.type = INT;
//                 break;
//             case FlOAT:
//                 serialize(row[index].get<float>(), f.data, configs[index - 1].size);
//                 break;
//             case TEXT:
//                 serialize(row[index].get<std::string>(), f.data, configs[index - 1].size);
//                 f.type = TEXT;
//                 break;
//             case BOOL:
//                 serialize(row[index].get<bool>(), f.data, configs[index - 1].size);
//                 f.type = BOOL;
//                 break;
//             }
//             feature_row.push_back(f);
//         }
//         features.push_back(feature_row);
//     }
// }
