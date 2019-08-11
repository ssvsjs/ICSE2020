#include <bits/stdc++.h>
#include <unistd.h>
using namespace std;
#include "inc.h"

const int ACTVER = 3;
const double OLOADRATE = 0.05;
const double alpha = 0.1;
//const double beta = 1;
const double OMAINTRATE = OLOADRATE;

const double eps = 1e-5;

inline int dcmp(double x, double y = 0)
{
    x -= y;
    return int(x > eps) - int(x < -eps);
}

struct Netflow {
    struct edge_t {
        int from, to;
        double flow;
        edge_t *last, *next;
        edge_t *flip;
    };

    vector<edge_t *> edge;
    int N, S, T;

    vector<int> dis;
    static const int inf;

    void edge_add(edge_t *&head, edge_t *e)
    {
        e->last = NULL;
        e->next = head;
        if (head)
            head->last = e;
        head = e;
    }
    bool buildMap()
    {
        fill(dis.begin(), dis.end(), inf);
        dis[S] = 0;
        queue<int> q;
        q.push(S);
        vector<bool> inq(N);
        fill(inq.begin(), inq.end(), false);
        inq[S] = true;
        while (!q.empty()) {
            int fr = q.front();
            q.pop();
            inq[fr] = false;
            for (edge_t *i = edge[fr]; i; i = i->next) {
                int t = i->to;
                if (dcmp(i->flow) > 0 && dis[t] > dis[fr] + 1) {
                    dis[t] = dis[fr] + 1;
                    if (!inq[t]) {
                        inq[t] = true;
                        q.push(t);
                    }
                }
            }
        }
        return dis[T] < inf;
    }
    vector<bool> vis;
    vector<edge_t *> sta, ecur;
    bool dfs(int x)
    {
        if (x == T)
            return true;
        vis[x] = true;
        for (edge_t *&i = ecur[x]; i; i = i->next)
            if (dcmp(i->flow) > 0 && !vis[i->to] && dis[i->to] == dis[x] + 1)
                if (dfs(i->to)) {
                    sta.push_back(i);
                    return true;
                }
        return false;
    }
    Netflow(int n, int s, int t)
        : edge(n, NULL), N(n), S(s), T(t), dis(n) {}
    void addEdge(int from, int to, double flow)
    {
        edge_t *e1, *e2;
        e1 = new edge_t;
        e2 = new edge_t;
        e1->from = from, e1->to = to, e1->flow = flow;
        e2->from = to, e2->to = from, e2->flow = 0;
        e1->flip = e2, e2->flip = e1;
        edge_add(edge[from], e1);
        edge_add(edge[to], e2);
    }
    double maxFlow()
    {
        double ans = 0;
        vis.resize(N);
        while (buildMap()) {
            ecur = edge;
            while (1) {
                sta.clear();
                fill(vis.begin(), vis.end(), false);
                if (!dfs(S))
                    break;
                double cur = inf;
                for (edge_t *i : sta)
                    cur = min(cur, i->flow);
                for (edge_t *i : sta) {
                    i->flow -= cur;
                    i->flip->flow += cur;
                }
                ans += cur;
            }
        }
        return ans;
    }
    double minCut(vector<bool> &c)
    {
        double ans = maxFlow();
        c.resize(N);
        for (int i = 0; i < N; ++i)
            c[i] = (dis[i] < inf);
        return ans;
    }
};
const int Netflow::inf = 1e7;

struct DenseMap {
    struct edge_t {
        int from, to;
        double weight;
        edge_t(int _from, int _to, double _w): from(_from), to(_to), weight(_w) {}
    };
    int n;
    vector<edge_t> e;
    vector<double> wnode;
    vector<bool> nec, necn;
    double sum;
	//vector<double> du;
    DenseMap(int _n)
        : n(_n), wnode(n), nec(n), necn(n) {}
    void setNecNode(int x)
    {
        assert(x < n);
        nec[x] = true;
    }
    void setNecNot(int x)
    {
        assert(x < n);
        necn[x] = true;
    }
    void setNodeWeight(int x, double w)
    {
        assert(x < n);
        wnode[x] = w;
    }
    void addEdge(int u, int v, double w)
    {
        assert(u < n);
        assert(v < n);
        e.emplace_back(u, v, w);
    }
    void init(double alpha)
    {
        for (edge_t &i : e)
			i.weight *= 1 - alpha;
		for (int i = 0; i < n; ++i)
			wnode[i] *= alpha;
		/*
		du = vector<double>(n, 0);
        for (const edge_t &i : e) {
			du[i.from] += i.weight;
			du[i.to] += i.weight;
		}
		*/
        sum = 0;
        for (double x : wnode)
            sum += x;
        for (const edge_t &i : e)
            sum += i.weight;
    }
    double checkans(double g, vector<bool> &nd)
    {
        int m = e.size();
        int S = 2 * n + m, T = S + 1;
        Netflow f(T + 1, S, T);
        for (int i = 0; i < m; ++i) {
            f.addEdge(S, n + i, e[i].weight);
            f.addEdge(n + i, n + m + e[i].from, Netflow::inf);
            f.addEdge(n + i, n + m + e[i].to, Netflow::inf);
        }
        for (int i = 0; i < n; ++i) {
            assert(!(nec[i] && necn[i]));
            f.addEdge(S, i, nec[i] ? Netflow::inf : wnode[i]);
            f.addEdge(i, n + m + i, Netflow::inf);
            f.addEdge(n + m + i, T, necn[i] ? Netflow::inf : g);
        }
        vector<bool> c;
        f.minCut(c);
        double ans = 0;
        for (int i = 0; i < n; ++i)
            if (c[i]) {
                ans += wnode[i];
                ans -= g;
            }
        for (int i = 0; i < m; ++i)
            if (c[n + i])
                ans += e[i].weight;
        nd.resize(n);
        for (int i = 0; i < n; ++i)
            nd[i] = c[i];
        return ans;
    }
    double maxdense(vector<bool> &nd)
    {
        int m = e.size();
        double l = 0, r = sum;
        while (r - l > eps) {
            double mid = (l + r) / 2;
            if (dcmp(checkans(mid, nd)) <= 0)
                r = mid;
            else
                l = mid;
        }
        checkans(l, nd);
        double ans = 0;
        int count = 0;
        for (int i = 0; i < n; ++i)
            if (nd[i])
                ++count;
        for (int i = 0; i < n; ++i)
            if (nd[i])
                ans += wnode[i];
        for (int i = 0; i < m; ++i)
            if (nd[e[i].from] && nd[e[i].to])
                ans += e[i].weight;
        ans /= count;
        return ans;
    }
	/*
    double cover(const vector<bool> &nd)
    {
        vector<double> cover(n, 0);
        for (const edge_t &i : e)
            if (nd[i.from] || nd[i.to]) {
                cover[i.from] += i.weight;
                cover[i.to] += i.weight;
            }
        int ct1 = 0, ct2 = 0;
        for (int i = 0; i < n; ++i)
            if (!nd[i]) {
                ++ct1;
                if (cover[i] > du[i] * 0.3)
                    ++ct2;
            }
		if (ct1 == 0)
			return 0;
        return (double)ct2 / ct1;
    }
    double dense(vector<bool> &nd)
    {
        double mxd = maxdense(nd);
        double cvd = cover(nd);
        const int maxIter = 100;
        for (int i = 0; i < maxIter; ++i) {
            double testd = mxd * (1 - (double(rand()) / RAND_MAX) * 0.2);
            vector<bool> nd2;
            double td = checkans(testd, nd2);
			assert(td >= 0);
            double tmp = cover(nd2);
            if (tmp > cvd) {
                cvd = tmp;
                nd = move(nd2);
            }
        }
		return cvd;
    }
	*/
};

string cwd;
void getcwd()
{
    static char _cwd[PATH_MAX];
    assert(getcwd(_cwd, sizeof(_cwd)) != NULL);
    cwd = _cwd;
}

int nau, nsy, nol;
map<int, int> contrib[MAXAUTH];
int syscont[MAXSYS];
vector<int> maint[MAXSYS], actdev[MAXSYS];
int loadmt[MAXAUTH];
double load[MAXSYS];
vector<pair<int, int>> relate[MAXAUTH];
string authname[MAXAUTH], sysname[MAXSYS];
vector<int> oload;
vector<bool> ismaint, isomaint;
bool isdriver[MAXSYS];
map<int, int> review[MAXAUTH];

void read_contrib()
{
    ifstream fin(cwd + "/dever.txt");
    fin >> nau;
    int t, x, y;
    for (int i = 0; i < nau; ++i) {
        getline(fin >> ws, authname[i]);
        fin >> t;
        for (int j = 0; j < t; ++j) {
            fin >> x >> y;
            contrib[i][x] = y;
			syscont[x] += y;
        }
    }
    fin.close();
    ismaint.resize(nau);
    fill(ismaint.begin(), ismaint.end(), false);
}

void read_subsys()
{
    ifstream fin(cwd + "/subsys.txt");
    fin >> nsy;
    int t, x, y, z;
    for (int i = 0; i < nsy; ++i) {
        getline(fin >> ws, sysname[i]);
        fin >> isdriver[i];
        fin >> t;
        map<int, pair<int, int>> dever;
        for (int j = 0; j < t; ++j) {
            fin >> x >> y >> z;
            dever.emplace(x, make_pair(y, z));
        }
        fin >> t;
        maint[i].resize(t);
        double sum = 0;
        for (int j = 0; j < t; ++j) {
            fin >> x >> y;
            maint[i][j] = x;
            dever[x] = make_pair(INT_MAX, date_before);
            loadmt[x] += y;
			review[x][i] += y;
            sum += y;
            ismaint[x] = true;
        }
        load[i] = t == 0 ? 0 : sum / t;
        for (auto j : dever)
            if (j.second.first >= ACTVER && j.second.second >= date_before - 3 * secpermonth)
                actdev[i].push_back(j.first);
    }
    fin.close();
}

void read_relate()
{
    ifstream fin(cwd + "/relate.txt");
    for (int i = 0; i < nau; ++i) {
        int m;
        fin >> m;
        relate[i].resize(m);
        for (int j = 0; j < m; ++j)
            fin >> relate[i][j].first >> relate[i][j].second;
    }
    fin.close();
}

void gen_overload()
{
    typedef pair<int, double> pid;
    vector<pid> ov;
    for (int i = 0; i < nsy; ++i)
		if (isdriver[i])
			ov.emplace_back(i, load[i]);
    sort(ov.begin(), ov.end(), [](const pid & a, const pid & b) {return a.second > b.second;});
    nol = ov.size() * OLOADRATE;
    oload.resize(nol);
    for (int i = 0; i < nol; ++i)
        oload[i] = ov[i].first;

    isomaint = vector<bool>(nau, false);
    vector<int> lmt;
    for (int i = 0; i < nau; ++i)
        if (ismaint[i])
            lmt.push_back(loadmt[i]);
    sort(lmt.begin(), lmt.end(), greater<int>());
    int th = lmt[int(lmt.size() * OMAINTRATE)];
    for (int i = 0; i < nau; ++i)
        if (ismaint[i] && loadmt[i] > th)
            isomaint[i] = true;
    cout << "maintainer overload threshold " << th << endl;
}

void run(int sysno)
{
    if (!isdriver[sysno]) {
        cout << '\t' << "not driver" << endl;
        return;
    }

    int n = actdev[sysno].size();
    DenseMap f(n);
    for (int i = 0; i < n; ++i) {
        int x = actdev[sysno][i];
        double c = contrib[x][sysno];
		//c += beta * review[x][sysno];
        f.setNodeWeight(i, c);
        if (isomaint[x] && find(maint[sysno].begin(), maint[sysno].end(), x) == maint[sysno].end())
            f.setNecNot(i);
    }
    for (int i : maint[sysno]) {
        auto it = lower_bound(actdev[sysno].begin(), actdev[sysno].end(), i);
        assert(it != actdev[sysno].end() && *it == i);
        f.setNecNode(it - actdev[sysno].begin());
    }
    for (int i = 0; i < n; ++i)
        for (auto j : relate[actdev[sysno][i]]) {
            auto it = lower_bound(actdev[sysno].begin(), actdev[sysno].end(), j.first);
            int x = it - actdev[sysno].begin();
            if (x <= i)
                continue;
            if (it != actdev[sysno].end() && *it == j.first)
                f.addEdge(i, x, j.second);
        }
    /* alpha * contribution + (1 - alpha) * relation */
    f.init(alpha);

    vector<bool> nd;
    double ans = f.maxdense(nd);
    int m = 0;
    for (int i = 0; i < n; ++i)
        if (nd[i])
            m++;
    cout << '\t' << "overload = " << load[sysno] << endl;
    cout << '\t' << "density = " << ans / syscont[sysno] * 100 << endl;
    cout << '\t' << "maintainers = {";
    for (int i = 0; i < maint[sysno].size(); ++i) {
        cout << authname[maint[sysno][i]];
        if (i + 1 < maint[sysno].size())
            cout << ", ";
    }
    cout << "}" << endl;
    if (m == maint[sysno].size()) {
        cout << '\t' << "no candidate committers" << endl;
        return;
    }
    cout << '\t' << "commiters = {";
    for (int i = 0, j = 0; i < n; ++i)
        if (nd[i]) {
            cout << authname[actdev[sysno][i]];
            if (++j < m)
                cout << ", ";
        }
    cout << "}" << endl;
}

int main()
{
    getcwd();

    read_contrib();
    read_subsys();
    read_relate();

    cout << "top subsystem with rate " << OLOADRATE << endl;
    gen_overload();

	for (int i = 0; i < oload.size(); ++i) {
		//if (sysname[oload[i]].find("INTEL DRM") == string::npos) continue;
		cout << endl << "rank " << i+1 << endl;
        cout << sysname[oload[i]] << endl;
        run(oload[i]);
    }
    return 0;
}
