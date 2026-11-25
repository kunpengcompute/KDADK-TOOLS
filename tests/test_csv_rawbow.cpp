// Linux: c++ -std=c++11 -Iinclude -Idemo tests/test_csv_rawbow.cpp -o /tmp/test_csv_rawbow
// Run: /tmp/test_csv_rawbow
#include "../demo/kdadk_file_writer.cpp"
#include <cassert>
#include <algorithm>

static std::string serialize(const std::string &payload, bool json = false)
{
    FILE *fp = tmpfile();
    assert(fp);
    feature_vector fv = {};
    rawbow rb = {};
    rb.len = static_cast<uint32_t>(payload.size());
    memcpy(rb.rawbow, payload.data(), std::min(payload.size(), sizeof(rb.rawbow)));
    int result = json ? write_json_feature_row_with_rawbow(fp, &fv, &rb, true)
                      : write_csv_feature_row_with_rawbow(fp, &fv, &rb);
    assert(result == WRITER_SUCCESS);
    rewind(fp);
    std::string output;
    char buffer[1024];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        output.append(buffer, n);
    }
    fclose(fp);
    return output;
}

static std::string csv_field(const std::string &payload)
{
    std::string row = serialize(payload);
    size_t start = row.find('"');
    assert(start != std::string::npos);
    assert(std::count(row.begin(), row.begin() + start, ',') == FEATURES_NUM - 1);
    return row.substr(start);
}

int main()
{
    for (const std::string &s : {"=1+1", "+1", "-1", "@SUM(1)", "\t=1", " =1", "\r=1", "\n=1"}) {
        assert(csv_field(s).substr(0, 2) == "\"'");
    }
    assert(csv_field("") == "\"\"\n");
    assert(csv_field("hello") == "\"hello\"\n");
    assert(csv_field("a,\"b\"\n\\") == "\"a,\"\"b\"\"\\n\\\\\"\n");
    assert(csv_field("=\"a,b\"") == "\"'=\"\"a,b\"\"\"\n");
    assert(csv_field(std::string("\0=1", 3)) == "\"\"\n");
    assert(csv_field(std::string(RAWBOW_MAX_LEN, '=')) ==
           "\"'" + std::string(RAWBOW_MAX_LEN, '=') + "\"\n");
    assert(serialize("=1", true).find("\"rawbow\": \"=1\"") != std::string::npos);
}
