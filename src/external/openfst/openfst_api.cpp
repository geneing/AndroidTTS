//
// Created by genei on 7/9/2024.
//

#include "openfst_api.h"


#include <fst/fst.h>
#include "fst/extensions/far/farlib.h"
#include "fst/extensions/mpdt/compose.h"
#include "fst/fst.h"
#include "fst/properties.h"
#include "fst/const-fst.h"
#include "fst/shortest-path.h"

namespace fst {
    // This variable is copied from
    // https://github.com/pzelasko/Pynini/blob/master/src/stringcompile.h#L81
    constexpr uint64_t kCompiledStringProps =
            kAcceptor | kIDeterministic | kODeterministic | kILabelSorted |
            kOLabelSorted | kUnweighted | kAcyclic | kInitialAcyclic | kTopSorted |
            kAccessible | kCoAccessible | kString | kUnweightedCycles;
}

class TextNormalizer {
public:

    TextNormalizer() = default;
    explicit TextNormalizer(std::unique_ptr<fst::StdConstFst> rule)
            : rule_(std::move(rule)) {}

    // @param s The input text to be normalized
    // @param remove_output_zero True to remove bytes whose value is 0 from the
    //                           output.

    [[nodiscard]] std::string Normalize(const std::string &s,
                                        bool remove_output_zero=true) const {
        // Step 1: Convert the input text into an FST
        fst::StdVectorFst text = StringToFst(s);

        // Step 2: Compose the input text with the rule FST
        fst::StdVectorFst composed_fst;
        fst::Compose(text, *rule_, &composed_fst);

        // Step 3: Get the best path from the composed FST
        fst::StdVectorFst one_best;
        fst::ShortestPath(composed_fst, &one_best, 1);

        return FstToString(one_best, remove_output_zero);
    }

private:
    std::unique_ptr<fst::StdConstFst> rule_;
    static fst::StdVectorFst StringToFst(const std::string &text) {
        using Weight = typename fst::StdArc::Weight;
        using Arc = fst::StdArc;

        fst::StdVectorFst ans;
        ans.ReserveStates(text.size());

        auto s = ans.AddState();
        ans.SetStart(s);
        // CAUTION(fangjun): We need to use uint8_t here.
        for (const uint8_t label : text) {
            const auto nextstate = ans.AddState();
            ans.AddArc(s, Arc(label, label, Weight::One(), nextstate));
            s = nextstate;
        }

        ans.SetFinal(s, Weight::One());
        ans.SetProperties(fst::kCompiledStringProps, fst::kCompiledStringProps);

        return ans;
    }

    static std::string FstToString(const fst::StdVectorFst &fst,
                                   bool remove_output_zero) {
        std::string ans;

        using Weight = typename fst::StdArc::Weight;
        using Arc = fst::StdArc;
        auto s = fst.Start();
        if (s == fst::kNoStateId) {
            // this is an empty FST
            return "";
        }
        while (fst.Final(s) == Weight::Zero()) {
            fst::ArcIterator<fst::Fst<Arc>> aiter(fst, s);
            if (aiter.Done()) {
                // not reached final.
                return "";
            }

            const auto &arc = aiter.Value();
            if (arc.olabel != 0 || !remove_output_zero) {
                ans.push_back(arc.olabel);
            }

            s = arc.nextstate;
            if (s == fst::kNoStateId) {
                // Transition to invalid state";
                return "";
            }

            aiter.Next();
            if (!aiter.Done()) {
                // not a linear FST
                return "";
            }
        }
        return ans;
    }
};

class FST {

    std::vector<std::unique_ptr<TextNormalizer>> tn_list_;
    static void SplitStringToVector(const std::string &full, const char *delim,
                                    bool omit_empty_strings,
                                    std::vector<std::string> *out) {
        size_t start = 0, found = 0, end = full.size();
        out->clear();
        while (found != std::string::npos) {
            found = full.find_first_of(delim, start);
            // start != end condition is for when the delimiter is at the end
            if (!omit_empty_strings || (found != start && start != end))
                out->push_back(full.substr(start, found - start));
            start = found + 1;
        }
    }

    static fst::ConstFst<fst::StdArc> *CastOrConvertToConstFst(fst::Fst<fst::StdArc> *fst) {
        // This version currently supports ConstFst<StdArc> or VectorFst<StdArc>
        std::string real_type = fst->Type();
        assert(real_type == "vector" || real_type == "const");
        if (real_type == "const") {
            return dynamic_cast<fst::ConstFst<fst::StdArc> *>(fst);
        } else {
            // As the 'fst' can't cast to VectorFst, we create a new
            // VectorFst<StdArc> initialized by 'fst', and delete 'fst'.
            auto *new_fst = new fst::ConstFst<fst::StdArc>(*fst);
            delete fst;
            return new_fst;
        }
    }

public:
    explicit FST(const std::string& far_list) {
        std::vector<std::string> files;
        SplitStringToVector(far_list, ",", false, &files);

        tn_list_.reserve(files.size() + tn_list_.size());

        for (const auto &f : files) {
            std::unique_ptr<fst::FarReader<fst::StdArc>> reader(fst::FarReader<fst::StdArc>::Open(f));
            for (; !reader->Done(); reader->Next()) {
                std::unique_ptr<fst::StdConstFst> r(
                        CastOrConvertToConstFst(reader->GetFst()->Copy()));

                tn_list_.push_back(
                        std::make_unique<TextNormalizer>(std::move(r)));
            }
        }
    }

    std::string Normalize(const std::string& text) {
        std::string textout = text;

        if (!tn_list_.empty()) {
            for (const auto &tn : tn_list_) {
                textout = tn->Normalize(textout);
                // std::cout << textout << std::endl;
            }
        }
        return textout;
    }
};

Normalizer::Normalizer(const std::string& far_list) {
    pFST = new FST( far_list );
}

Normalizer::~Normalizer(){
    delete pFST;
}

std::string Normalizer::apply(const std::string &text) {
    return pFST->Normalize(text);
}
//
//int main( void )
//{
//
//    std::string far_file = "/home/eingerman/Projects/TTS/NeMo-text-processing/tools/text_processing_deployment/en_tn_True_deterministic_cased__tokenize.far";
//    std::string far_file_postprocess="/home/eingerman/Projects/TTS/NeMo-text-processing/tools/text_processing_deployment/en_tn_post_processing.far";
//    std::string far_file_verbalize="/home/eingerman/Projects/TTS/NeMo-text-processing/tools/text_processing_deployment/en_tn_True_deterministic_verbalizer.far";
//
//    FST fst(far_file + "," + far_file_verbalize+","+far_file_postprocess);
//    // FST fst("/home/eingerman/Projects/TTS/NeMo-text-processing/tools/text_processing_deployment/en_tn_grammars_cased/en_tn_True_deterministic_cased__tokenize.far");
//    // FST fst(
//    //     "/home/eingerman/Projects/TTS/NeMo-text-processing/tools/"
//    //     "text_processing_deployment/en_tn_grammars_cased/classify/"
//    //     "tokenize_and_classify.far,/home/eingerman/"
//    //     "Projects/TTS/"
//    //     "NeMo-text-processing/tools/text_processing_deployment/"
//    //     "en_tn_grammars_cased/verbalize/verbalize.far,/home/eingerman/Projects/"
//    //     "TTS/"
//    //     "NeMo-text-processing/tools/text_processing_deployment/"
//    //     "en_tn_grammars_cased/verbalize/post_process.far");
//    // FST fst("");
////   FST fst(
////       "/home/eingerman/Projects/TTS/NeMo-text-processing/tools/"
////       "text_processing_deployment/en_tn_grammars_cased/verbalize/"
////       "post_process.far");
//
//    std::string text =
//            "2788 San Tomas Expy, Santa Clara, CA 95051. 708 N 1st St, San City. "
//            "Today is 17 may 2010. Tomorrow is 10/06/2005. The total bill was "
//            "$20.01. He weights 4 1/2 lbs.";
//    std::cout << fst.Normalize(text) << std::endl;
//
//    return 0;
//}