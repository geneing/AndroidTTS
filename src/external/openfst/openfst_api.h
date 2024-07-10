//
// Created by genei on 7/9/2024.
//

#ifndef ANDROIDTTS_OPENFST_API_H
#define ANDROIDTTS_OPENFST_API_H
#include <vector>
#include <string>

class FST;

class Normalizer
{
private:
    FST* pFST;
public:
    explicit Normalizer( const std::string& far_list );
    std::string apply( const std::string& text );
    ~Normalizer();
};
#endif //ANDROIDTTS_OPENFST_API_H
