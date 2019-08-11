#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
using namespace std;
#include "inc.h"

bool isdir(const char *path)
{
    struct stat statbuf;
    if (stat(path, &statbuf) != 0)
        return 0;
    return S_ISDIR(statbuf.st_mode);
}

string strip_name(const string &name)
{
    int n = name.length();
    if (n >= 2 && name[0] == '"' && name[n - 1] == '"')
        return name.substr(1, n - 2);
    return name;
}

class NameMan
{
    map<string, int> mp;
    vector<string> nm;
    int sz;
public:
    NameMan() : sz(0) {}
    int size() const
    {
        return sz;
    }
    string get(int x) const
    {
        return nm[x];
    }
    int get(const string &name) const
    {
        auto it = mp.find(name);
        if (it == mp.end())
            return -1;
        return it->second;
    }
    int put(const string &name)
    {
        auto it = mp.lower_bound(name);
        if (it != mp.end() && it->first == name)
            return it->second;
        mp.insert(it, make_pair(name, sz));
        nm.push_back(name);
        return sz++;
    }
};

NameMan subsysman, deverman;
string cwd;
vector<int> sysmaints[MAXSYS];
vector<int> sysdever[MAXSYS];

struct Tdir {
    string fullname;
    set<int> subsys;
    bool isdir;
    map<string, Tdir *> son;
    Tdir(const string &name, bool _isdir) : fullname(name), isdir(_isdir) {}
} *droot;

string linux_dir;

vector<int> versions;
set<int> actver[MAXAUTH];
map<int, int> lastact[MAXAUTH];
map<int, int> contrib[MAXAUTH]; // contribution to a subsystem
map<int, int> relate[MAXAUTH];
map<int, int> review[MAXAUTH];
map<string, vector<int>> commitsys;
bool isdriver[MAXSYS];

template<class Iterator>
Tdir *search_file(Tdir *h, Iterator begin, Iterator end)
{
    if (begin == end)
        return h;
    auto it = h->son.find(*begin);
    if (it == h->son.end())
        return NULL;
    return search_file(it->second, begin + 1, end);
}

Tdir *dirscan(const string &dir, string name = "")
{
    bool isd = isdir(dir.c_str());
    if (isd && !name.empty())
        name += '/';
    Tdir *h = new Tdir(name, isd);
    if (!h->isdir)
        return h;
    struct dirent *pDirent;
    DIR *pDir = opendir(dir.c_str());
    assert(pDir != NULL);
    while ((pDirent = readdir(pDir)) != NULL) {
        auto name2 = pDirent->d_name;
        if (name2[0] == '.')
            continue;
        h->son[name2] = dirscan(dir + "/" + name2, name + name2);
    }
    closedir(pDir);
    return h;
}

vector<string> parsepath(const string &str)
{
    vector<string> ans;
    int last = -1;
    while (1) {
        int pos = str.find('/', last + 1);
        if (pos == string::npos)
            break;
        ans.push_back(str.substr(last + 1, pos - last - 1));
        last = pos;
    }
    ans.push_back(str.substr(last + 1));
    return ans;
}

/* s1 is in the pattern of s2 */
bool pathmatch(const string &s1, const string &s2)
{
    const int MAXL = 70;
    int n = s1.length(), m = s2.length();
    assert(n < MAXL && m < MAXL);
    static bool f[MAXL][MAXL];
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < m; ++j)
            f[i][j] = false;
    for (int i = 0; i < m; ++i)
        if (s2[i] == '*') {
            if (i == 0)
                for (int j = 0; j < n; ++j)
                    f[j][i] = true;
            else {
                int j = 0;
                while (j < n && !f[j][i - 1])
                    ++j;
                while (j < n)
                    f[j++][i] = true;
            }
        }
        else
            for (int j = 0; j < n; ++j)
                if (s1[j] == s2[i]) {
                    if (i == 0 && j == 0)
                        f[j][i] = true;
                    else if (i != 0 && j != 0 && f[j - 1][i - 1])
                        f[j][i] = true;
                }
    return f[n - 1][m - 1];
}

template <class Iterator>
void setsubsys(int sysno, Tdir *h, Iterator begin, Iterator end)
{
    if (!h->isdir && (begin == end || begin->empty()))
        h->subsys.insert(sysno);
    if (begin == end) {
        if (h->isdir)
            for (auto i = h->son.begin(); i != h->son.end(); ++i)
                setsubsys(sysno, i->second, begin, end);
        return;
    }
    for (auto i = h->son.begin(); i != h->son.end(); ++i)
        if (begin->empty())
            setsubsys(sysno, i->second, begin, end);
        else if (pathmatch(i->first, *begin))
            setsubsys(sysno, i->second, begin + 1, end);
}

void parse_maintainer()
{
    ifstream fin(linux_dir + "/MAINTAINERS");
    string line;
    int subsys = -1;
    bool start = false;
    while (getline(fin, line)) {
        if (line.find("-----------------------------------") != string::npos) {
            start = true;
            continue;
        }
        if (!start)
            continue;
        if (line.empty())
            continue;
        if (line[1] == ':') {
            assert(subsys >= 0);
            string data = line.substr(3);
            if (line[0] == 'M') { // maintainer
                int mpos = data.find('<');
                string name;
                if (mpos == -1)
                    continue;
                else
                    name = data.substr(0, mpos - 1);
                name = strip_name(name);
                int maint = deverman.put(name);
                sysmaints[subsys].push_back(maint);
            }
            if (line[0] == 'F') { // files
                vector<string> lst = parsepath(data);
                setsubsys(subsys, droot, lst.begin(), lst.end());
                if (data.find("drivers") != string::npos)
                    isdriver[subsys] = true;
            }
        }
        else
            subsys = subsysman.put(line);
    }
    fin.close();
}

void parse_version()
{
    ifstream fin(cwd + "/version.txt");
    string str;
    while (getline(fin, str))
        versions.push_back(stoi(str));
    fin.close();
    reverse(versions.begin(), versions.end());
    versions.push_back(INT_MAX);
}

map<string, string> namechange;

string final_name(const string &name)
{
    if (namechange[name] == "")
        return name;
    return final_name(namechange[name]);
}

void get_name_changed(const string &cname, string &name, string &oldname)
{
    size_t p = cname.find(" => ");
    if (p == string::npos) {
        name = cname;
        oldname = cname;
        return;
    }
    static regex ename0("(.*) => (.*)");
    static regex ename1("\\{(.*) => (.*)\\}");
    const regex &ename = cname.find('{') == string::npos ? ename0 : ename1;
    name = regex_replace(cname, ename, "$2");
    oldname = regex_replace(cname, ename, "$1");
    static regex wdir("//");
    name = regex_replace(name, wdir, "/");
    oldname = regex_replace(oldname, wdir, "/");
    if (final_name(oldname) != final_name(name))
        namechange[oldname] = name;
}

void parse_contrib()
{
    regex ecmt("commit ([a-f0-9]*) : (.*), (\\d*)");
    regex efile("\\d*\\t\\d*\\t(.*)");
    ifstream fin(cwd + "/nstat.txt");
    string line;
    int author = -1;
    vector<int> vec;
    string hash;
    int timestamp = today;
    auto pushvec = [&]() {
        if (author < 0 || vec.empty())
            return;
        sort(vec.begin(), vec.end());
        auto ed = unique(vec.begin(), vec.end());
        for (auto i = vec.begin(); i != ed; ++i) {
            contrib[author][*i]++;
            int &tmp = lastact[author][*i];
            tmp = max(tmp, timestamp);
        }
        vector<int> &t = commitsys[hash];
        t.insert(t.end(), vec.begin(), ed);
    };
    long nofd = 0, totc = 0;
    while (getline(fin, line)) {
        smatch sm;
        if (regex_match(line, sm, ecmt)) {
            pushvec();
            vec.clear();
            timestamp = stoi(sm[3]);
            if (timestamp > date_before || timestamp < date_after)
                continue;
            hash = sm[1];
            string name = strip_name(sm[2]);
            author = deverman.put(name);
            assert(author < MAXAUTH);
            int ver = lower_bound(versions.begin(), versions.end(), timestamp) - versions.begin();
            actver[author].insert(ver);
        }
        else {
            if (timestamp > date_before || timestamp < date_after)
                continue;
            if (regex_match(line, sm, efile)) {
                assert(author >= 0);
                ++totc;
                string cname = sm[1];
                string name, oldname;
                get_name_changed(cname, name, oldname);
                name = final_name(name);
                vector<string> lst = parsepath(name);
                Tdir *node = search_file(droot, lst.begin(), lst.end());
                if (node == NULL)
                    ++nofd;
                else
                    vec.insert(vec.end(), node->subsys.begin(), node->subsys.end());
            }
        }
    }
    cout << (double)nofd / totc << " file not found" << endl;
    fin.close();
    pushvec();
}

void parse_relate()
{
    ifstream fin(cwd + "/comlog.txt");
    regex ecmt("commit ([a-f0-9]*) : (.*), (\\d*)");
    regex erel("(.*): (.*) <.*@.*>");
    string line;
    vector<int> auths;
    auto pushauth = [&]() {
        for (auto i = auths.begin(); i != auths.end(); ++i)
            for (auto j = auths.begin(); j != auths.end(); ++j)
                if (*i != *j)
                    relate[*i][*j]++;
    };
    int timestamp = today;
    int rauth = -1;
    string hash;
    while (getline(fin, line)) {
        smatch sm;
        if (regex_match(line, sm, ecmt)) {
            pushauth();
            auths.clear();
            string name = strip_name(sm[2]);
            rauth = deverman.put(name);
            assert(rauth < MAXAUTH);
            timestamp = stoi(sm[3]);
            hash = sm[1];
        }
        else {
            if (timestamp > date_before || timestamp < date_after)
                continue;
            if (regex_match(line, sm, erel)) {
                string name = strip_name(sm[2]);
                int author = deverman.put(name);
                assert(author < MAXAUTH);
                for (int i : commitsys[hash]) {
                    int &tmp = lastact[author][i];
                    tmp = max(tmp, timestamp);
                }
                auths.push_back(author);
                if (sm[1] == "Signed-off-by" && author != rauth)
                    for (int i : commitsys[hash])
                        review[author][i]++;
            }
        }
    }
    fin.close();
    pushauth();
}

void getcwd()
{
    static char _cwd[PATH_MAX];
    assert(getcwd(_cwd, sizeof(_cwd)) != NULL);
    cwd = _cwd;
}

void gen_dever()
{
    ofstream fout(cwd + "/dever.txt");
    fout << deverman.size() << endl;
    for (int i = 0; i < deverman.size(); ++i) {
        fout << deverman.get(i) << endl;
        fout << contrib[i].size() << endl;
        for (const auto &j : contrib[i])
            fout << j.first << ' ' << j.second << endl;
    }
    fout.close();
}

void gen_subsys()
{
    for (int i = 0; i < deverman.size(); ++i)
        for (const auto &j : contrib[i])
            sysdever[j.first].push_back(i);
    ofstream fout(cwd + "/subsys.txt");
    fout << subsysman.size() << endl;
    for (int i = 0; i < subsysman.size(); ++i) {
        fout << subsysman.get(i) << endl;
        fout << isdriver[i] << endl;
        fout << sysdever[i].size() << endl;
        for (int j : sysdever[i])
            fout << j << ' ' << actver[j].size() << ' ' << lastact[j][i] << endl;
        fout << sysmaints[i].size() << endl;
        for (int j : sysmaints[i])
            fout << j << ' ' << review[j][i] << endl;
    }
    fout.close();
}

void gen_relate()
{
    ofstream fout(cwd + "/relate.txt");
    int n = deverman.size();
    for (int i = 0; i < n; ++i) {
        fout << relate[i].size() << endl;
        for (auto j : relate[i])
            fout << j.first << ' ' << j.second << endl;
        fout << endl;
    }
    fout.close();
}

void gitrevert()
{
    string hash = "git log --all --before=" + to_string(date_before) + " -1 --pretty=format:%H";
    string cmd = "cd " + linux_dir + "; git checkout `" + hash + "`";
    system(cmd.c_str());
}

void print_relate(const string &name)
{
    int x = deverman.get(name);
    if (x == -1)
        cout << name << " is not a developer" << endl;
    else
        cout << name << " relation:" << endl;
    for (auto i : relate[x])
        cout << deverman.get(i.first) << ' ' << i.second << endl;
}

int main(int argc, char *argv[])
{
    assert(argc == 2);
    assert(isdir(argv[1]));
    linux_dir = argv[1];
    getcwd();

    cout << "retrieve data between:" << endl;
    system(("date -d @" + to_string(date_after)).c_str());
    system(("date -d @" + to_string(date_before)).c_str());

    cout << "reverting git" << endl;
    gitrevert();

    cout << "scaning linux directory" << endl;
    droot = dirscan(linux_dir);

    cout << "parsing maintainers" << endl;
    parse_maintainer();

    cout << "parsing version" << endl;
    parse_version();

    cout << "parsing contribution" << endl;
    parse_contrib();

    cout << "parsing relation" << endl;
    parse_relate();

    cout << "developers: " << deverman.size() << endl;

    cout << "generating developer" << endl;
    gen_dever();

    cout << "generating subsystem" << endl;
    gen_subsys();

    cout << "generating relation" << endl;
    gen_relate();

    return 0;
}
