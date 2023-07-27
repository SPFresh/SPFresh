#include <limits>
#include <string>
#include <sys/types.h>

#include "inc/Core/Common.h"
#include "inc/Core/Common/DistanceUtils.h"
#include "inc/Core/Common/QueryResultSet.h"
#include "inc/Core/SPANN/Index.h"
#include "inc/Helper/VectorSetReader.h"
#include "inc/Helper/StringConvert.h"
#include "inc/SSDServing/Utils.h"
#include "inc/Core/Common/TruthSet.h"

using namespace SPTAG;

class ToolOptions : public Helper::ReaderOptions
{
public:
    ToolOptions():Helper::ReaderOptions(VectorValueType::Float, 0, VectorFileType::TXT, "|", 32)
    {
        AddOptionalOption(newDataSetFileName, "-NewDataSetFileName", "--NewDataSetFileName", "1");
        AddOptionalOption(currentListFileName, "-CurrentListFileName", "--CurrentListFileName", "1");
        AddOptionalOption(reserveListFileName, "-ReserveListFileName", "--ReserveListFileName", "1");
        AddOptionalOption(traceFileName, "-TraceFileName", "--TraceFileName", "1");
        AddOptionalOption(baseNum, "-BaseNum", "--BaseNum", "1");
        AddOptionalOption(reserveNum, "-ReserveNum", "--ReserveNum", "1");
        AddOptionalOption(updateSize, "-UpdateSize", "--UpdateSize", "1");
        AddOptionalOption(batch, "-Batch", "--Batch", "1");
        AddOptionalOption(genTrace, "-GenTrace", "--GenTrace", "1");
        AddOptionalOption(convertTruth, "-ConvertTruth", "--ConvertTruth", "1");
        AddOptionalOption(callRecall, "-CallRecall", "--CallRecall", "1");
        AddOptionalOption(genSet, "-GenSet", "--GenSet", "1");
        AddOptionalOption(genStress, "-GenStress", "--GenStress", "1");
        AddOptionalOption(m_vectorPath, "-VectorPath", "--VectorPath", "1");
        AddOptionalOption(m_distCalcMethod, "-m", "--dist", "Distance method (L2 or Cosine).");
        AddOptionalOption(m_resultNum, "-resultNum", "--resultNum", "1");
        AddOptionalOption(m_querySize, "-querySize", "--querySize", "1");
        AddOptionalOption(m_truthPath, "-truthPath", "--truthPath", "1");
        AddOptionalOption(m_truthType, "-truthType", "--truthType", "1");
        AddOptionalOption(m_queryPath, "-queryPath", "--queryPath", "1");
        AddOptionalOption(m_searchResult, "-searchResult", "--searchResult", "1");
    }

    ~ToolOptions() {}

    std::string newDataSetFileName;
    std::string currentListFileName;
    std::string reserveListFileName;
    std::string traceFileName;
    std::string m_vectorPath;
    int baseNum;
    int reserveNum;
    int updateSize;
    int batch;
    bool genTrace;
    bool convertTruth;
    bool callRecall;
    bool genSet;
    bool genStress;
    DistCalcMethod m_distCalcMethod = DistCalcMethod::L2;
    int m_resultNum;
    int m_querySize;
    std::string m_truthPath;
    TruthFileType m_truthType;
    std::string m_queryPath;
    std::string m_searchResult;
    std::string m_headVectorFile;
    std::string m_headIDFile;
    
} options;

template<typename T, typename V>
void PrintPercentiles(const std::vector<V>& p_values, std::function<T(const V&)> p_get, const char* p_format, bool reverse=false)
{
    double sum = 0;
    std::vector<T> collects;
    collects.reserve(p_values.size());
    for (const auto& v : p_values)
    {
        T tmp = p_get(v);
        sum += tmp;
        collects.push_back(tmp);
    }

    if (reverse) {
        std::sort(collects.begin(), collects.end(), std::greater<T>());
    }
    else {
        std::sort(collects.begin(), collects.end());
    }
    if (reverse) {
        LOG(Helper::LogLevel::LL_Info, "Avg\t50tiles\t90tiles\t95tiles\t99tiles\t99.9tiles\tMin\n");
    }
    else {
        LOG(Helper::LogLevel::LL_Info, "Avg\t50tiles\t90tiles\t95tiles\t99tiles\t99.9tiles\tMax\n");
    }

    std::string formatStr("%.3lf");
    for (int i = 1; i < 7; ++i)
    {
        formatStr += '\t';
        formatStr += p_format;
    }

    formatStr += '\n';

    LOG(Helper::LogLevel::LL_Info,
        formatStr.c_str(),
        sum / collects.size(),
        collects[static_cast<size_t>(collects.size() * 0.50)],
        collects[static_cast<size_t>(collects.size() * 0.90)],
        collects[static_cast<size_t>(collects.size() * 0.95)],
        collects[static_cast<size_t>(collects.size() * 0.99)],
        collects[static_cast<size_t>(collects.size() * 0.999)],
        collects[static_cast<size_t>(collects.size() - 1)]);
}

void LoadTruth(ToolOptions& p_opts, std::vector<std::set<SizeType>>& truth, int numQueries, std::string truthfilename, int truthK)
{
    auto ptr = f_createIO();
    LOG(Helper::LogLevel::LL_Info, "Start loading TruthFile...: %s\n", truthfilename.c_str());
    
    if (ptr == nullptr || !ptr->Initialize(truthfilename.c_str(), std::ios::in | std::ios::binary)) {
        LOG(Helper::LogLevel::LL_Error, "Failed open truth file: %s\n", truthfilename.c_str());
        exit(1);
    }
    LOG(Helper::LogLevel::LL_Error, "K: %d, TruthResultNum: %d\n", p_opts.m_resultNum, p_opts.m_resultNum);    
    COMMON::TruthSet::LoadTruth(ptr, truth, numQueries, p_opts.m_resultNum, p_opts.m_resultNum, p_opts.m_truthType);
    char tmp[4];
    if (ptr->ReadBinary(4, tmp) == 4) {
        LOG(Helper::LogLevel::LL_Error, "Truth number is larger than query number(%d)!\n", numQueries);
    }
}

std::shared_ptr<VectorSet>  LoadVectorSet(ToolOptions& p_opts, int numThreads)
{
    std::shared_ptr<VectorSet> vectorSet;
    LOG(Helper::LogLevel::LL_Info, "Start loading VectorSet...\n");
    if (!p_opts.m_vectorPath.empty() && fileexists(p_opts.m_vectorPath.c_str())) {
        std::shared_ptr<Helper::ReaderOptions> vectorOptions(new Helper::ReaderOptions(p_opts.m_inputValueType, p_opts.m_dimension, p_opts.m_inputFileType, p_opts.m_vectorDelimiter));
        auto vectorReader = Helper::VectorSetReader::CreateInstance(vectorOptions);
        if (ErrorCode::Success == vectorReader->LoadFile(p_opts.m_vectorPath))
        {
            vectorSet = vectorReader->GetVectorSet();
            if (p_opts.m_distCalcMethod == DistCalcMethod::Cosine) vectorSet->Normalize(numThreads);
            LOG(Helper::LogLevel::LL_Info, "\nLoad VectorSet(%d,%d).\n", vectorSet->Count(), vectorSet->Dimension());
        }
    }
    return vectorSet;
}

std::shared_ptr<VectorSet> LoadQuerySet(ToolOptions& p_opts)
{
    LOG(Helper::LogLevel::LL_Info, "Start loading QuerySet...\n");
    std::shared_ptr<Helper::ReaderOptions> queryOptions(new Helper::ReaderOptions(p_opts.m_inputValueType, p_opts.m_dimension, p_opts.m_inputFileType, p_opts.m_vectorDelimiter));
    auto queryReader = Helper::VectorSetReader::CreateInstance(queryOptions);
    if (ErrorCode::Success != queryReader->LoadFile(p_opts.m_queryPath))
    {
        LOG(Helper::LogLevel::LL_Error, "Failed to read query file.\n");
        exit(1);
    }
    return queryReader->GetVectorSet();
}

void LoadOutputResult(ToolOptions& p_opts, std::vector<std::vector<SizeType>>& ids,
    std::vector<std::vector<float>>& dists) 
{
    auto ptr = f_createIO();
    if (ptr == nullptr || !ptr->Initialize(p_opts.m_searchResult.c_str(), std::ios::binary | std::ios::in)) {
        LOG(Helper::LogLevel::LL_Error, "Failed open file: %s\n", p_opts.m_searchResult.c_str());
        exit(1);
    }
    int32_t NumQuerys;
    if (ptr->ReadBinary(sizeof(NumQuerys), (char*)&NumQuerys) != sizeof(NumQuerys)) {
        LOG(Helper::LogLevel::LL_Error, "Fail to read result file!\n");
        exit(1);
    }
    int32_t resultNum;
    if (ptr->ReadBinary(sizeof(resultNum), (char*)&resultNum) != sizeof(resultNum)) {
        LOG(Helper::LogLevel::LL_Error, "Fail to read result file!\n");
        exit(1);
    }


    for (size_t i = 0; i < NumQuerys; ++i)
    {
        std::vector<SizeType> tempVec_id;
        std::vector<float> tempVec_dist;
        for (int j = 0; j < resultNum; ++j)
        {
            int32_t i32Val;
            if (ptr->ReadBinary(sizeof(i32Val), (char*)&i32Val) != sizeof(i32Val)) {
                LOG(Helper::LogLevel::LL_Error, "Fail to read result file!\n");
                exit(1);
            }

            float fVal;
            if (ptr->ReadBinary(sizeof(fVal), (char*)&fVal) != sizeof(fVal)) {
                LOG(Helper::LogLevel::LL_Error, "Fail to read result file!\n");
                exit(1);
            }
            tempVec_id.push_back(i32Val);
            tempVec_dist.push_back(fVal);
        }
        ids.push_back(tempVec_id);
        dists.push_back(tempVec_dist);
    }
}

void LoadUpdateMapping(std::string fileName, std::vector<SizeType>& reverseIndices)
{
    LOG(Helper::LogLevel::LL_Info, "Loading %s\n", fileName.c_str());

    int vectorNum;

    auto ptr = f_createIO();
    if (ptr == nullptr || !ptr->Initialize(fileName.c_str(), std::ios::in | std::ios::binary)) {
        LOG(Helper::LogLevel::LL_Error, "Failed open trace file: %s\n", fileName.c_str());
        exit(1);
    }
    
    if (ptr->ReadBinary(4, (char *)&vectorNum) != 4) {
        LOG(Helper::LogLevel::LL_Error, "vector Size Error!\n");
    }

    reverseIndices.clear();
    reverseIndices.resize(vectorNum);

    if (ptr->ReadBinary(vectorNum * 4, (char*)reverseIndices.data()) != vectorNum * 4) {
        LOG(Helper::LogLevel::LL_Error, "update mapping Error!\n");
        exit(1);
    }
}

void SaveUpdateMapping(std::string fileName, std::vector<SizeType>& reverseIndices, SizeType vectorNum)
{
    LOG(Helper::LogLevel::LL_Info, "Saving %s\n", fileName.c_str());

    auto ptr = f_createIO();
    if (ptr == nullptr || !ptr->Initialize(fileName.c_str(), std::ios::out | std::ios::binary)) {
        LOG(Helper::LogLevel::LL_Error, "Failed open trace file: %s\n", fileName.c_str());
        exit(1);
    }
    
    if (ptr->WriteBinary(4, reinterpret_cast<char*>(&vectorNum)) != 4) {
        LOG(Helper::LogLevel::LL_Error, "vector Size Error!\n");
    }

    for (int i = 0; i < vectorNum; i++) {
        if (ptr->WriteBinary(4, reinterpret_cast<char*>(&reverseIndices[i])) != 4) {
            LOG(Helper::LogLevel::LL_Error, "update mapping Error!\n");
            exit(1);
        }
    }
}

void SaveUpdateTrace(std::string fileName, SizeType& updateSize, std::vector<SizeType>& insertSet, std::vector<SizeType>& deleteSet)
{
    LOG(Helper::LogLevel::LL_Info, "Saving %s\n", fileName.c_str());

    auto ptr = f_createIO();
    if (ptr == nullptr || !ptr->Initialize(fileName.c_str(), std::ios::out | std::ios::binary)) {
        LOG(Helper::LogLevel::LL_Error, "Failed open trace file: %s\n", fileName.c_str());
        exit(1);
    }
    
    if (ptr->WriteBinary(4, reinterpret_cast<char*>(&updateSize)) != 4) {
        LOG(Helper::LogLevel::LL_Error, "vector Size Error!\n");
        exit(1);
    }

    for (int i = 0; i < updateSize; i++) {
        if (ptr->WriteBinary(4, reinterpret_cast<char*>(&deleteSet[i])) != 4) {
            LOG(Helper::LogLevel::LL_Error, "vector Size Error!\n");
            exit(1);
        }
    }

    for (int i = 0; i < updateSize; i++) {
        if (ptr->WriteBinary(4, reinterpret_cast<char*>(&insertSet[i])) != 4) {
            LOG(Helper::LogLevel::LL_Error, "vector Size Error!\n");
            exit(1);
        }
    }
}

void InitializeList(std::vector<SizeType>& current_list, std::vector<SizeType>& reserve_list, ToolOptions& p_opts)
{
    LOG(Helper::LogLevel::LL_Info, "Initialize list\n");
    if (p_opts.batch == 0) {
        current_list.resize(p_opts.baseNum);
        reserve_list.resize(p_opts.reserveNum);
        for (int i = 0; i < p_opts.baseNum; i++) current_list[i] = i;
        for (int i = 0; i < p_opts.reserveNum; i++) reserve_list[i] = i+p_opts.baseNum;
    } else {
        LoadUpdateMapping(p_opts.currentListFileName + std::to_string(p_opts.batch-1), current_list);
        LoadUpdateMapping(p_opts.reserveListFileName + std::to_string(p_opts.batch-1), reserve_list);
    }
}


template <typename ValueType>
void GenerateTrace(ToolOptions& p_opts)
{
    LOG(Helper::LogLevel::LL_Info, "Begin Generating Trace\n");
    std::shared_ptr<VectorSet> vectorSet;
    std::vector<SizeType> current_list;
    std::vector<SizeType> reserve_list;
    InitializeList(current_list, reserve_list, p_opts);
    LOG(Helper::LogLevel::LL_Info, "Loading Vector Set\n");
    if (!p_opts.m_vectorPath.empty() && fileexists(p_opts.m_vectorPath.c_str())) {
        std::shared_ptr<Helper::ReaderOptions> vectorOptions(new Helper::ReaderOptions(p_opts.m_inputValueType, p_opts.m_dimension, p_opts.m_inputFileType, p_opts.m_vectorDelimiter));
        auto vectorReader = Helper::VectorSetReader::CreateInstance(vectorOptions);
        if (ErrorCode::Success == vectorReader->LoadFile(p_opts.m_vectorPath))
        {
            vectorSet = vectorReader->GetVectorSet();
            if (p_opts.m_distCalcMethod == DistCalcMethod::Cosine) vectorSet->Normalize(4);
            LOG(Helper::LogLevel::LL_Info, "\nLoad VectorSet(%d,%d).\n", vectorSet->Count(), vectorSet->Dimension());
        }
    }
    LOG(Helper::LogLevel::LL_Info, "batch: %d, updateSize: %d\n", p_opts.batch, p_opts.updateSize);
    std::vector<SizeType> deleteList;
    std::vector<SizeType> insertList;
    deleteList.resize(p_opts.updateSize);
    insertList.resize(p_opts.updateSize);
    LOG(Helper::LogLevel::LL_Info, "Generate delete list\n");
    std::shuffle(current_list.begin(), current_list.end(), rg);
    for (int j = 0; j < p_opts.updateSize; j++) {
        deleteList[j] = current_list[p_opts.baseNum-j-1];
    }
    current_list.resize(p_opts.baseNum-p_opts.updateSize);
    LOG(Helper::LogLevel::LL_Info, "Generate insert list\n");
    std::shuffle(reserve_list.begin(), reserve_list.end(), rg);
    for (int j = 0; j < p_opts.updateSize; j++) {
        insertList[j] = reserve_list[p_opts.reserveNum-j-1];
        current_list.push_back(reserve_list[p_opts.reserveNum-j-1]);
    }

    reserve_list.resize(p_opts.reserveNum-p_opts.updateSize);
    for (int j = 0; j < p_opts.updateSize; j++) {
        reserve_list.push_back(deleteList[j]);
    }

    LOG(Helper::LogLevel::LL_Info, "Sorting list\n");
    std::sort(current_list.begin(), current_list.end());
    std::sort(reserve_list.begin(), reserve_list.end());

    SaveUpdateMapping(p_opts.currentListFileName + std::to_string(p_opts.batch), current_list, p_opts.baseNum);
    SaveUpdateMapping(p_opts.reserveListFileName + std::to_string(p_opts.batch), reserve_list, p_opts.reserveNum);

    SaveUpdateTrace(p_opts.traceFileName + std::to_string(p_opts.batch), p_opts.updateSize, insertList, deleteList);

    LOG(Helper::LogLevel::LL_Info, "Generate new dataset for truth\n");
    COMMON::Dataset<ValueType> newSample(0, p_opts.m_dimension, 1024 * 1024, MaxSize);

    for (int i = 0; i < p_opts.baseNum; i++) {
        newSample.AddBatch(1, (ValueType*)(vectorSet->GetVector(current_list[i])));
    }

    LOG(Helper::LogLevel::LL_Info, "Saving Truth\n");
    newSample.Save(p_opts.newDataSetFileName + std::to_string(p_opts.batch));
}

template <typename ValueType>
void GenerateStressTest(ToolOptions& p_opts)
{
    LOG(Helper::LogLevel::LL_Info, "Begin Generating Stress Test\n");
    std::shared_ptr<VectorSet> vectorSet;
    std::vector<SizeType> current_list;
    LOG(Helper::LogLevel::LL_Info, "batch: %d, updateSize: %d\n", p_opts.batch, p_opts.updateSize);
    current_list.resize(p_opts.baseNum);
    for (int i = 0; i < p_opts.baseNum; i++) current_list[i] = i;
    for (int i = 0; i < p_opts.batch; i++) {
        std::vector<SizeType> insertList;
        insertList.resize(p_opts.updateSize);
        std::shuffle(current_list.begin(), current_list.end(), rg);
        LOG(Helper::LogLevel::LL_Info, "Generate delete/re-insert list\n");
        for (int j = 0; j < p_opts.updateSize; j++) {
            insertList[j] = current_list[p_opts.baseNum-j-1];
        }
        std::string fileName = p_opts.traceFileName + std::to_string(i);
        auto ptr = f_createIO();
        if (ptr == nullptr || !ptr->Initialize(fileName.c_str(), std::ios::out | std::ios::binary)) {
            LOG(Helper::LogLevel::LL_Error, "Failed open trace file: %s\n", fileName.c_str());
            exit(1);
        }
        if (ptr->WriteBinary(4, reinterpret_cast<char*>(&p_opts.updateSize)) != 4) {
            LOG(Helper::LogLevel::LL_Error, "vector Size Error!\n");
            exit(1);
        }

        for (int i = 0; i < p_opts.updateSize; i++) {
            if (ptr->WriteBinary(4, reinterpret_cast<char*>(&insertList[i])) != 4) {
                LOG(Helper::LogLevel::LL_Error, "vector Size Error!\n");
                exit(1);
            }
        }
    }
}

template <typename ValueType>
void ConvertTruth(ToolOptions& p_opts)
{
    std::vector<SizeType> current_list;
    LoadUpdateMapping(p_opts.currentListFileName + std::to_string(p_opts.batch), current_list);
    int K = p_opts.m_resultNum;

    int numQueries = p_opts.m_querySize;

    std::string truthFile = p_opts.m_truthPath + std::to_string(p_opts.batch);
    std::vector<std::vector<SizeType>> truth;
    LOG(Helper::LogLevel::LL_Info, "Start loading TruthFile...\n");
    truth.resize(numQueries);
    auto ptr = f_createIO();
    if (ptr == nullptr || !ptr->Initialize(truthFile.c_str(), std::ios::in | std::ios::binary)) {
        LOG(Helper::LogLevel::LL_Error, "Fail to read the file:%s\n", truthFile.c_str());
        exit(1);
    }

    if (ptr->TellP() == 0) {
        int row, originalK;
        if (ptr->ReadBinary(4, (char*)&row) != 4 || ptr->ReadBinary(4, (char*)&originalK) != 4) {
            LOG(Helper::LogLevel::LL_Error, "Fail to read truth file!\n");
            exit(1);
        }
    }

    for (int i = 0; i < numQueries; ++i)
    {
        truth[i].clear();
        truth[i].resize(K);
        if (ptr->ReadBinary(K * 4, (char*) truth[i].data()) != K * 4) {
            LOG(Helper::LogLevel::LL_Error, "Truth number(%d) and query number(%d) are not match!\n", i, numQueries);
            exit(1);
        }
    }

    LOG(Helper::LogLevel::LL_Info, "ChangeMapping & Writing\n");
    std::string truthFileAfter = p_opts.m_truthPath + "_after" + std::to_string(p_opts.batch);
    ptr = SPTAG::f_createIO();
    if (ptr == nullptr || !ptr->Initialize(truthFileAfter.c_str(), std::ios::out | std::ios::binary)) {
        LOG(Helper::LogLevel::LL_Error, "Fail to create the file:%s\n", truthFileAfter.c_str());
        exit(1);
    }
    ptr->WriteBinary(4, (char*)&numQueries);
    ptr->WriteBinary(4, (char*)&K);
    for (int i = 0; i < numQueries; i++) {
        for (int j = 0; j < K ; j++) {
            if (ptr->WriteBinary(4, (char*)(&current_list[truth[i][j]])) != 4) {
                LOG(Helper::LogLevel::LL_Error, "Fail to write the truth file!\n");
                exit(1);
            }
        }
    }
}

template <typename ValueType>
void CallRecall(ToolOptions& p_opts)      
{
    std::string truthfile = p_opts.m_truthPath;
    std::vector<std::set<SizeType>> truth;
    int truthK = p_opts.m_resultNum;
    int K = p_opts.m_resultNum;
    auto vectorSet = LoadVectorSet(p_opts, 10);
    auto querySet = LoadQuerySet(p_opts);
    int NumQuerys = querySet->Count();
    LoadTruth(p_opts, truth, NumQuerys, truthfile, truthK);
    std::vector<std::vector<SizeType>> ids;
    std::vector<std::vector<float>> dists;

    LoadOutputResult(p_opts,ids, dists);

    float meanrecall = 0, minrecall = MaxDist, maxrecall = 0, stdrecall = 0;
    std::vector<float> thisrecall(NumQuerys, 0);
    std::unique_ptr<bool[]> visited(new bool[K]);
    LOG(Helper::LogLevel::LL_Info, "Start Calculating Recall\n");
    for (SizeType i = 0; i < NumQuerys; i++)
    {
        memset(visited.get(), 0, K * sizeof(bool));
        for (SizeType id : truth[i])
        {
            for (int j = 0; j < K; j++)
            {
                if (visited[j] || ids[i][j] < 0) continue;
                if (vectorSet != nullptr) {
                    float dist = dists[i][j];
                    float truthDist = COMMON::DistanceUtils::ComputeDistance((const ValueType*)querySet->GetVector(i), (const ValueType*)vectorSet->GetVector(id), vectorSet->Dimension(), SPTAG::DistCalcMethod::L2);
                    if (fabs(dist - truthDist) < Epsilon * (dist + Epsilon)) {
                        thisrecall[i] += 1;
                        visited[j] = true;
                        break;
                    }
                }
            }
        }
        thisrecall[i] /= truthK;
        meanrecall += thisrecall[i];
        if (thisrecall[i] < minrecall) minrecall = thisrecall[i];
        if (thisrecall[i] > maxrecall) maxrecall = thisrecall[i];
    }
    meanrecall /= NumQuerys;
    for (SizeType i = 0; i < NumQuerys; i++)
    {
        stdrecall += (thisrecall[i] - meanrecall) * (thisrecall[i] - meanrecall);
    }
    stdrecall = std::sqrt(stdrecall / NumQuerys);

    LOG(Helper::LogLevel::LL_Info, "stdrecall: %.6lf, maxrecall: %.2lf, minrecall: %.2lf\n", stdrecall, maxrecall, minrecall);

    LOG(Helper::LogLevel::LL_Info, "\nRecall Distribution:\n");
    PrintPercentiles<float, float>(thisrecall,
        [](const float recall) -> float
        {
            return recall;
        },
        "%.3lf", true);

    LOG(Helper::LogLevel::LL_Info, "Recall%d@%d: %f\n", K, truthK, meanrecall);
}


int main(int argc, char* argv[]) {
    if (!options.Parse(argc - 1, argv + 1))
    {
        exit(1);
    }
    if (options.genTrace) {
        switch (options.m_inputValueType) {
        case SPTAG::VectorValueType::Float:
            GenerateTrace<float>(options);
            break;
        case SPTAG::VectorValueType::Int16:
            GenerateTrace<std::int16_t>(options);
            break;
        case SPTAG::VectorValueType::Int8:
            GenerateTrace<std::int8_t>(options);
            break;
        case SPTAG::VectorValueType::UInt8:
            GenerateTrace<std::uint8_t>(options);
            break;
        default:
            LOG(Helper::LogLevel::LL_Error, "Error data type!\n");
        }
    }

    if (options.genStress) {
        switch (options.m_inputValueType) {
        case SPTAG::VectorValueType::Float:
            GenerateStressTest<float>(options);
            break;
        case SPTAG::VectorValueType::Int16:
            GenerateStressTest<std::int16_t>(options);
            break;
        case SPTAG::VectorValueType::Int8:
            GenerateStressTest<std::int8_t>(options);
            break;
        case SPTAG::VectorValueType::UInt8:
            GenerateStressTest<std::uint8_t>(options);
            break;
        default:
            LOG(Helper::LogLevel::LL_Error, "Error data type!\n");
        }
    }

    if (options.convertTruth) {
        switch (options.m_inputValueType) {
        case SPTAG::VectorValueType::Float:
            ConvertTruth<float>(options);
            break;
        case SPTAG::VectorValueType::Int16:
            ConvertTruth<std::int16_t>(options);
            break;
        case SPTAG::VectorValueType::Int8:
            ConvertTruth<std::int8_t>(options);
            break;
        case SPTAG::VectorValueType::UInt8:
            ConvertTruth<std::uint8_t>(options);
            break;
        default:
            LOG(Helper::LogLevel::LL_Error, "Error data type!\n");
        }
    }

    if (options.callRecall) {
        switch (options.m_inputValueType) {
        case SPTAG::VectorValueType::Float:
            CallRecall<float>(options);
            break;
        case SPTAG::VectorValueType::Int16:
            CallRecall<std::int16_t>(options);
            break;
        case SPTAG::VectorValueType::Int8:
            CallRecall<std::int8_t>(options);
            break;
        case SPTAG::VectorValueType::UInt8:
            CallRecall<std::uint8_t>(options);
            break;
        default:
            LOG(Helper::LogLevel::LL_Error, "Error data type!\n");
        }
    }
    
    return 0;
}