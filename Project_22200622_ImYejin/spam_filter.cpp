#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <regex>
#include <map>
#include <unordered_set>

using namespace std;
typedef map<string, int>::iterator Map;

// 특수문자 삭제
bool is_word(string& word) {
    regex specialChar("[^a-zA-Z]");
    word = regex_replace(word, specialChar, "");
    return !word.empty();
}

void csv_dictionary(string& file, string& storage) {
    ifstream input_file(file);
    ofstream output_dict(storage);
    map<string, int> word_cnt; // 단어가 등장한 메일 개수 저장
    string line;
    bool in_spam_subject = false;
    unordered_set<string> exist_words; 

    //디버깅
    if (!input_file.is_open()) {
        cout << "Failed to open input file!" << endl;
        return;
    }
    if (!output_dict.is_open()) {
        cout << "Failed to open output file!" << endl;
        return;
    }

    //파일 읽기 실행
    while (getline(input_file, line)) {
        stringstream ss(line);
        string word;

        //spamSubject 기준으로 메일 하나를 구분
        if ((line.find("spam,\"Subject:") != string::npos) || (line.find("ham,\"Subject:") != string::npos)) {
            if (in_spam_subject) { // 새 spamSubejct일 경우 (-> 새로운 메일)
                exist_words.clear();
            }
            in_spam_subject = true;
        }

        //단어 추적
        if (in_spam_subject) {
            while (ss >> word) {
                if (is_word(word) &&
                    word != "numlabeltext" && word != "spamSubject" && word != "hamSubject" &&
                    word != "a" && word != "the" && word != "in" && word != "by" &&
                    word != "of" && word != "and" && word != "is" && word != "are" &&
                    word != "as" && word != "at" && word != "be" && word != "an" &&
                    word != "for" && word != "from" && word != "has" && word != "have") {
                    
                    if (exist_words.find(word) == exist_words.end()) {
                        word_cnt[word]++; // 새로운 단어가 등장하면 개수 증가 (중복단어 무시)
                        exist_words.insert(word);
                    }
                }
            }
        }
    }

    //csv에 저장 (단어 | 개수)
    for (Map it = word_cnt.begin(); it != word_cnt.end(); ++it) {
        output_dict << it->first << "," << it->second << endl;
    }

    input_file.close();
    output_dict.close();
}

void calculate_r(string& test_file, map<string, int>& spam_dict, map<string, 
                int>& ham_dict, string& reject_storage, float threshold, string& result_storage) {
    ifstream input_file(test_file);
    ofstream reject_file(reject_storage);
    ofstream result_file(result_storage);
    string line;
    float max_r = 0.0; // 각 행에 대한 최대 r_value
    int will_spam = 0; // 한자리 단어 나올때마다 증가 (한자리 단어 많이 나올수록 스팸일 확률 증가)
    int will_ham = 0; // etc 단어 나올때마다 증가 (etc 단어 나올수록 스팸 아닐 확률 증가)

    reject_file << "word,r(word)" << endl;

    bool subject_found = false;  // Subject가 처음 발견되면 이후부터 텍스트를 처리하도록 하는 플래그
    string current_text = ""; // 현재까지 읽은 텍스트 (한 이메일의 내용)

    while (getline(input_file, line)) {
        // 맨 처음 spam,"Subject:" 구간이 나오기 전까지의 내용은 무시
        if (!subject_found && (line.find("spam,\"Subject:") != string::npos || line.find("ham,\"Subject:") != string::npos)) {
            subject_found = true;
            current_text = "";
            continue;
        }

        if (subject_found) {
            // spamSubject 발견 시 그 전에 저장된 내용을 처리
            if ((line.find("spam,\"Subject:") != string::npos) || (line.find("ham,\"Subject:") != string::npos)) {
                if (!current_text.empty()) {
                    // r(word) 계산
                    float max_r = 0.0;
                    stringstream ss(current_text);
                    string word;

                    // 단어별 r(word) 계산
                    while (ss >> word) {
                        if (is_word(word)) {
                            transform(word.begin(), word.end(), word.begin(), ::tolower); // 대소문자 구분X
                            int spam_count, ham_count;

                            if (spam_dict.find(word) != spam_dict.end()) {
                                spam_count = spam_dict.at(word);
                            }
                            else 
                                spam_count = 0;

                            if (ham_dict.find(word) != ham_dict.end()) {
                                ham_count = ham_dict.at(word);
                            }
                            else 
                                ham_count = 0;
                            
                            // p(word)와 q(word) 계산 (둘 중에 하나가 없을 경우 확률은 0.5로 지정)
                            double p = spam_count / 100.0;
                            double q = ham_count / 100.0;
                            double r;
                            if (p == 0 || q == 0)
                                r = 0.5;
                            else if (p + q > 0){
                                r = p / (p + q);
                            }

                            // r(word) 값 저장
                            reject_file << word << "," << r << endl;
                             if (r > max_r) {
                                max_r = r;
                            }

                            // 정확도 높이기 위한 추가 설정
                            if (word.length() == 1) { // 1자리 단어는 will_spam 값을 증가
                                will_spam++;
                            }
                            if (word == "ect") { // 1자리 단어는 will_spam 값을 증가
                                will_ham++;
                            }
                        }
                    }

                    // 최대 r(word)와 threshold 비교
                    if ((max_r >= threshold || will_spam == 1) && will_ham < 2) {
                        result_file << max_r << ",spam" << endl;
                        cout << "spam" << endl;
                    } else{
                        result_file << max_r << ",ham" << endl;
                        cout << "ham" << endl;
                    }
                }

                // 새 메일내용 시작
                current_text = "";
            }

            // 내용 누적
            current_text += " " + line;
        }
    }

    // 마지막 내용 처리
    if (!current_text.empty()) {
        float max_r = 0.0;
        stringstream ss(current_text);
        string word;

        while (ss >> word) {
            if (is_word(word)) {
                transform(word.begin(), word.end(), word.begin(), ::tolower);
                int spam_count, ham_count;

                if (spam_dict.find(word) != spam_dict.end()) {
                    spam_count = spam_dict.at(word);
                }
                else 
                    spam_count = 0;

                if (ham_dict.find(word) != ham_dict.end()) {
                    ham_count = ham_dict.at(word);
                }
                else 
                    ham_count = 0;
                
                double p = spam_count / 100.0;
                double q = ham_count / 100.0;
                double r;
                if (p == 0 || q == 0)
                    r = 0.5;
                else if (p + q > 0){
                    r = p / (p + q);
                }

                reject_file << word << "," << r << endl;
                if (r > max_r) {
                    max_r = r;
                }

                if (word.length() == 1) {
                    will_spam++;
                }
                if (word == "ect") {
                    will_ham++;
                }
            }
        }

        if ((max_r >= threshold || will_spam == 1) && will_ham < 2) {
            result_file << max_r << ",spam" << endl;
            cout << "spam" << endl;
        } else {
            result_file << max_r << ",ham" << endl;
            cout << "ham" << endl;
        }
    }

    // 파일 닫기
    input_file.close();
    reject_file.close();
}

void load_dictionary(string& file, map<string, int>& dict) {
    ifstream input_file(file);
    string word;
    int count;

    // 디버깅
    if (!input_file.is_open()) {
        cout << "Error opening file: " << file << endl;
        return;
    }

    // 파일에서 단어, 개수 읽기
    while (getline(input_file, word, ',') && input_file >> count) {
        transform(word.begin(), word.end(), word.begin(), ::tolower); //대소문자 구분X
        
        dict[word] = count; // map에 단어, 개수 추가
        input_file.ignore(numeric_limits<streamsize>::max(), '\n'); // 줄바꿈 문자 처리
    }

    input_file.close();
}

int main() {
    float threshold = 0.95; // 임계값
    string spam_file = "emails/train/dataset_spam_train100.csv";  // spam_train 파일 입력
    string spam_storage = "spam_dictionary.csv";
    string ham_file = "emails/train/dataset_ham_train100.csv";   // ham_train 파일 입력
    string ham_storage = "ham_dictionary.csv";
    string test_file = "emails/test/dataset_ham_test20.csv"; // test.csv 파일 입력
    string reject_storage = "reject.csv"; // reject.csv에 결과 저장
    string result_storage = "result.csv";

    csv_dictionary(spam_file, spam_storage); // spam 단어장 생성
    csv_dictionary(ham_file, ham_storage);   // ham 단어장 생성

    // 단어장 로드
    map<string, int> spam_dict;
    map<string, int> ham_dict;

    // spam_dict와 ham_dict에 단어와 개수를 저장
    load_dictionary(spam_storage, spam_dict); // spam 단어장 파일을 spam_dict에 로드
    load_dictionary(ham_storage, ham_dict);   // ham 단어장 파일을 ham_dict에 로드

    // r_values 계산 및 reject.csv, result.csv 저장
    calculate_r(test_file, spam_dict, ham_dict, reject_storage, threshold, result_storage);

    return 0;
}