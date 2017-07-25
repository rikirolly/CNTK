//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.md file in the project root for full license information.
//

#define _CRT_SECURE_NO_WARNINGS // "secure" CRT not available on all platforms  --add this at the top of all CPP files that give "function or variable may be unsafe" warnings

#include "CNTKLibrary.h"
//#include "Layers.h"
//#include "Common.h"
//#include "TimerUtility.h"

#include <cstdio>
#include <map>
#include <set>
#include <vector>

#define let const auto

using namespace CNTK;
using namespace std;

//using namespace Dynamite;

struct PlainTextVocabularyConfiguration
{
    const std::wstring fileName;
    const std::wstring insertAtStart;
    const std::wstring insertAtEnd;
    const std::wstring substituteForUnknown;
};

struct PlainTextStreamConfiguration : public StreamConfiguration
{
    PlainTextStreamConfiguration(const std::wstring& streamName, size_t dim, const std::vector<std::wstring>& fileNames,
                                 const PlainTextVocabularyConfiguration& vocabularyConfig,
                                 bool definesMbSize = false) :
        StreamConfiguration(streamName, dim, true, L"", definesMbSize),
        m_fileNames(fileNames), m_vocabularyConfig(vocabularyConfig)
    {
    }
    std::vector<std::wstring> m_fileNames;
    PlainTextVocabularyConfiguration m_vocabularyConfig;
};

// parameters needed:
//  - array of streams:
//     - dataFiles: main data file names--array of strings (possibly allow single string and array); allow wildcard  --check with Eldar on whether that is common
//        - it will be required that the #lines of files across streams matches 1:1, to discover bad wildcard expansion
//     - vocabularyFile:
//        - file name
//        - start sym to insert
//        - end sym to insert
//        - unk sym
//     - StreamConfiguration:
//        - m_streamName (key): stream name (for referring to it)
//        - m_dim --must match vocabulary size (augmented with start/end/unk sym if not present)
//        - m_format must be sparse
//        - m_definesMBSize--leave to user, whatever this means
//  - cacheIndex: should we cache? cache file derived from main file (one for each); base on date
static Deserializer PlainTextDeserializer(const std::vector<PlainTextStreamConfiguration>& streams, bool cacheIndex = true)
{
    Deserializer ctf;
    Dictionary input;
    for (const auto& s : streams)
    {
        const auto& key = s.m_streamName;
        Dictionary stream;
        // PlainTextDeserializer-specific fields
        std::vector<DictionaryValue> fileNames;
        for (let& n : s.m_fileNames)
            fileNames.push_back(n);
        stream[L"dataFiles"]            = fileNames;
        auto tmp = s.m_vocabularyConfig;
        stream[L"vocabularyFile"]       = s.m_vocabularyConfig.fileName;
        stream[L"insertAtStart"]        = s.m_vocabularyConfig.insertAtStart;
        stream[L"insertAtEnd"]          = s.m_vocabularyConfig.insertAtEnd;
        stream[L"substituteForUnknown"] = s.m_vocabularyConfig.substituteForUnknown;
        // standard fields
        stream[L"dim"] = s.m_dim;
        if (!s.m_isSparse || !s.m_streamAlias.empty())
            LogicError("PlainTextDeserializer got unexpected isSpare and/or streamAlias values.");
        stream[L"format"] = s.m_isSparse ? L"sparse" : L"dense";
        stream[L"definesMBSize"] = s.m_definesMbSize;
        input[key] = stream;
    }
    ctf.Add(L"type", L"PlainTextDeserializer", L"input", input, L"cacheIndex", cacheIndex);
    return ctf;
}


void Train(const DeviceDescriptor& device, bool useSparseLabels)
{
    let srcVocabSize = 10000;
    let tgtVocabSize = 10000;
    //const size_t inputDim = 2000;
    //const size_t embeddingDim = 500;
    //const size_t hiddenDim = 250;
    //const size_t attentionDim = 20;
    //const size_t numOutputClasses = 5;
    //
    //const wstring trainingCTFPath = L"C:/work/CNTK/Tests/EndToEndTests/Text/SequenceClassification/Data/Train.ctf";
    //
    //// static model and criterion function
    //auto model_fn = CreateModelFunction(numOutputClasses, embeddingDim, hiddenDim, device);
    //auto criterion_fn = CreateCriterionFunction(model_fn);
    //
    //// dynamic model and criterion function
    //auto d_model_fn = CreateModelFunctionUnrolled(numOutputClasses, embeddingDim, hiddenDim, device);
    //auto d_criterion_fn = CreateCriterionFunctionUnrolled(d_model_fn);

    // data
    let minibatchSource = CreateCompositeMinibatchSource(MinibatchSourceConfig({ PlainTextDeserializer(
        {
            PlainTextStreamConfiguration(L"src", srcVocabSize, { L"src.txt" }, { L"src.vocab", L"<s>", L"</s>", L"<unk/>" }),
            PlainTextStreamConfiguration(L"tgt", tgtVocabSize, { L"tgt.txt" }, { L"tgt.vocab", L"<s>", L"</s>", L"<unk/>" })
        })},
        /*randomize=*/false/*for now*/));
}

int mt_main(int argc, char *argv[])
{
    argc; argv;
    try
    {
        Train(DeviceDescriptor::GPUDevice(0), true);
        //Train(DeviceDescriptor::CPUDevice(), true);
    }
    catch (exception& e)
    {
        fprintf(stderr, "EXCEPTION caught: %s\n", e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}