#!/usr/bin/python
# -*- coding: UTF-8 -*-
from __future__ import division
from datetime import datetime
import pymysql
import numpy as np
import csv
from numpy import *
import math


def MAX_RW(start_time, end_time):
	month={}
	cursor.execute("SELECT month(author_date), count(distinct scmlog.hash) FROM "
		"scmlog, hash_file "
		"where author_date between %s and %s "
		"and scmlog.hash = hash_file.hash "
		"and hash_file.file_name like 'drivers/gpu/drm/i915%%' "
		"group by month(author_date), committer_name "
		"order by month(author_date) asc, count(distinct scmlog.hash) desc",
		(start_time, end_time))
	res = cursor.fetchall()
	for i in range(len(res)):
		if res[i][0] in month:
			month[res[i][0]].append(res[i][1])
		else:
			month[res[i][0]]=[res[i][1]]
	print([(month[i][0]) for i in month.keys()])
	return [(month[i][0]) for i in month.keys()], month

# M2: Intensity of review workload: the ratio of review workload done by the busiest maintainer or committer.
def INTST_RW(start_time, end_time):
	month=MAX_RW()[-1]
	cursor.execute("SELECT month(author_date), count(distinct scmlog.hash) FROM "
		"scmlog, hash_file "
		"where author_date between %s and %s "
		"and scmlog.hash = hash_file.hash "
		"and hash_file.file_name like 'drivers/gpu/drm/i915%%' "
		"group by month(author_date) ",
		(start_time, end_time))
	res = cursor.fetchall()
	for i in range(len(res)):
		if res[i][0] in month:
			month[res[i][0]].append(res[i][1])
		else:
			month[res[i][0]]=[0, res[i][1]]
	print([float((month[i][0])/month[i][-1])*200 for i in month.keys()])
	return [float((month[i][0])/month[i][-1]) for i in month.keys()]

# M3: Entropy of review workload: the dispersion of review workload assignment, borrowed from information theory.
def ENT_RW(start_time, end_time):
	month=self.MAX_RW()[-1]
	result=[]
	cursor.execute("SELECT month(author_date), count(distinct scmlog.hash) FROM "
		"scmlog, hash_file "
		"where author_date between %s and %s "
		"and scmlog.hash = hash_file.hash "
		"and hash_file.file_name like 'drivers/gpu/drm/i915%%' "
		"group by month(author_date) ",
		(start_time, end_time))
	res = cursor.fetchall()
	for i in range(len(res)):
		if month.has_key(res[i][0]):
			month[res[i][0]].append(res[i][1])
		else:
			month[res[i][0]]=[0, res[i][1]]
	
	for i in month.keys():
		shang=0
		for j in range(len(month[i])-1):
			p = month[i][j]/month[i][-1]
			shang += -p*math.log(p, 2)
		result.append(shang)
	return result

# M4: Latency of review workload: the median review time ofthe patches.
def LAT_RW(start_time, end_time):
	month={}
	cursor.execute("SELECT month(author_date), delay FROM "
		"(SELECT distinct scmlog.hash, author_date, DATEDIFF(commit_date, author_date) as delay "
		"FROM scmlog, hash_file  "
		"where author_date between %s and %s "
		"and scmlog.hash = hash_file.hash "
		"and hash_file.file_name like 'drivers/gpu/drm/i915%%') as tmp ",
		(start_time, end_time))
	res = cursor.fetchall()
	for i in range(len(res)):
		if res[i][0] in month:
			month[res[i][0]].append(res[i][1])
		else:
			month[res[i][0]]=[res[i][1]]
	print([np.median(month[i]) for i in month.keys()])
	return [np.median(month[i]) for i in month.keys()]

# M5: Ratio of workload outside the eight most productivehours per day.
def OVERWORK(start_time, end_time):
	month={}
	work=[]
	cursor.execute("SELECT month(author_date), count(distinct scmlog.hash), committer_name FROM "
		"scmlog, hash_file "
		"where author_date between %s and %s "
		"and scmlog.hash = hash_file.hash "
		"and hash_file.file_name like 'drivers/gpu/drm/i915%%' "
		"group by month(author_date), committer_name "
		"order by month(author_date) asc, count(distinct scmlog.hash) desc",
		(start_time, end_time))
	res = cursor.fetchall()
	for t in range(len(res)):
		if month.has_key(res[t][0]):
			month[res[t][0]].append(res[t][2])
		else:
			month[res[t][0]]=[res[t][2]]
	d={1:31, 2:28, 3:31, 4:30, 5:31, 6:30, 7:31, 8:31, 9:30, 10:31, 11:30, 12:31}
	year=start_time.strftime("%Y-%m-%d").split('-')[0]
	for j in range(1,13):
		extra_workload=0
		total_workload=0
		start_time=year+'-'+str(j)+'-'+'01'
		end_time=year+'-'+str(j)+'-'+str(d[j])
		try:
			for k in range(len(month[j])):
				cursor.execute("SELECT hour(author_date), count(distinct scmlog.hash)FROM "
					"scmlog, hash_file "
					"where author_date between %s and %s "
					"and scmlog.hash = hash_file.hash "
					"and hash_file.file_name like 'drivers/gpu/drm/i915%%' "
					"and committer_name like %s "
					"group by hour(author_date) "
					"order by count(distinct scmlog.hash) desc",
					(start_time, end_time, month[j][k]))
				res = cursor.fetchall()
				for f in range(len(res)):
					total_workload+=res[f][1]
					if f>=8:
						extra_workload+=res[f][1]
			if total_workload !=0:
				work.append(extra_workload/total_workload)
			else:
				work.append('None')
		except: work.append('None')
	return work

# M6: Complexity of review relationship: Among all the maintainers, the maximum number of unique authors reviewed by a certain maintainer.
def CPLX_RR(start_time, end_time):
	month={}
	work=[]
	cursor.execute("SELECT month(author_date), count(distinct author_name), committer_name FROM "
		"scmlog, hash_file "
		"where author_date between %s and %s "
		"and scmlog.hash = hash_file.hash "
		"and hash_file.file_name like 'drivers/gpu/drm/i915%%' "
		"group by month(author_date), committer_name "
		"order by month(author_date) asc, count(distinct author_name) desc",
		(start_time, end_time))
	res = cursor.fetchall()
	for t in range(len(res)):
		if month.has_key(res[t][0]):
			month[res[t][0]].append(res[t][1])
		else:
			month[res[t][0]]=[res[t][1]]
	for i in range(12):
		if month.has_key(i+1):
			work.append(month[i+1][0])
		else:
			work.append(0)
	return work

# M7: File complexity: Among all the maintainers, the maximum number of unique files reviewed by a certain maintainer.
def CPLX_F(start_time, end_time):
	month={}
	work=[]
	cursor.execute("SELECT month(author_date), count(distinct file_name), committer_name FROM "
		"scmlog, hash_file "
		"where author_date between %s and %s "
		"and scmlog.hash = hash_file.hash "
		"and hash_file.file_name like 'drivers/gpu/drm/i915%%' "
		"group by month(author_date), committer_name "
		"order by month(author_date) asc, count(distinct file_name) desc",
		(start_time, end_time))
	res = cursor.fetchall()
	for t in range(len(res)):
		if month.has_key(res[t][0]):
			month[res[t][0]].append(res[t][1])
		else:
			month[res[t][0]]=[res[t][1]]
	for i in range(12):
		if month.has_key(i+1):
			work.append(month[i+1][0])
		else:
			work.append(0)
	return work

# M8: We use the averagenumber of unique reviewers per patch (including the author)as the indicator of the effort/strategy for quality assurance. We retrieved all the tags that indicate some sort of review,including “Signed-off-by”, “Reviewed-by” and “Tested-by”
def QLTY_ANR(start_time, end_time):
	d={1:31, 2:28, 3:31, 4:30, 5:31, 6:30, 7:31, 8:31, 9:30, 10:31, 11:30, 12:31}    
	year=start_time.strftime("%Y-%m-%d").split('-')[0]
	work=[]
	for j in range(1,13):
		start_time=year+'-'+str(j)+'-'+'01'
		end_time=year+'-'+str(j)+'-'+str(d[j])
		try:
			cursor.execute("SELECT avg(num) FROM ( "
				"SELECT hash, count(hash) as num FROM ( "
				"SELECT distinct * FROM ( "
				"SELECT hash, name FROM review "
				"union all "
				"SELECT hash, name FROM ack "
				"union all "
				"SELECT hash, name FROM sign) as tmp where tmp.hash in ( "
				"SELECT distinct scmlog.hash FROM scmlog "
				"where author_date between %s and %s ) ) as tmp2 "
				"GROUP BY tmp2.hash ) as tmp3 ",
				(start_time, end_time))
			res = cursor.fetchall()
			work.append(res[0][0])
		except:
			continue
	return work
 
# M9: we define the ratio of self-commits (commits authored by maintainers or committers) reviewed by others asan indicator of strictness of review for patches contributed by contributors with commit rights.
def QLTY_RSRO(start_time, end_time):
	d={1:31, 2:28, 3:31, 4:30, 5:31, 6:30, 7:31, 8:31, 9:30, 10:31, 11:30, 12:31}
	year=start_time.strftime("%Y-%m-%d").split('-')[0]
	work=[]
	for j in range(1,13):
		start_time=year+'-'+str(j)+'-'+'01'
		end_time=year+'-'+str(j)+'-'+str(d[j])
		try:
			cursor.execute("SELECT count(*) FROM ( "
				"SELECT hash, count(hash) as num FROM ( "
				"SELECT distinct * FROM ( "
				"SELECT hash, name FROM review "
				"union all "
				"SELECT hash, name FROM ack "
				"union all "
				"SELECT hash, name FROM test "
				"union all "
				"SELECT hash, name FROM sign) as tmp where tmp.hash in ( "               
				"SELECT distinct scmlog.hash FROM scmlog, hash_file " 
				"where author_date between %s and %s "
				"and scmlog.hash = hash_file.hash "
				"and scmlog.author_name=scmlog.committer_name "
				"and hash_file.file_name like 'drivers/gpu/drm/i915%%')) as tmp2 "
				"GROUP BY tmp2.hash ) as tmp3 where num>=2 ",
				(start_time, end_time))
			res1 = cursor.fetchall()[0][0]

			cursor.execute("SELECT count(*) FROM ( "
				"SELECT hash, count(hash) as num FROM ( "
				"SELECT distinct * FROM ( "
				"SELECT hash, name FROM review "
				"union all "
				"SELECT hash, name FROM ack "
				"union all "
				"SELECT hash, name FROM test "
				"union all "
				"SELECT hash, name FROM sign) as tmp where tmp.hash in ( "               
				"SELECT distinct scmlog.hash FROM scmlog, hash_file " 
				"where author_date between %s and %s "
				"and scmlog.hash = hash_file.hash "
				"and scmlog.author_name=scmlog.committer_name "
				"and hash_file.file_name like 'drivers/gpu/drm/i915%%' )) as tmp2 "
				"GROUP BY tmp2.hash ) as tmp3 ",
				(start_time, end_time))
			res2 = cursor.fetchall()[0][0]
			t=res1/res2
			work.append(t)
		except: 
			continue
	return work

if __name__ == '__main__':
	conn = pymysql.connect(host='127.0.0.1', port=3306, user='root', passwd='4133728699', db='maintainer', charset='utf8')
	cursor = conn.cursor()
	version_date = ['2007-01-01','2008-01-01','2009-01-01','2010-01-01','2011-01-01','2012-01-01', '2013-01-01','2014-01-01','2015-01-01','2016-01-01','2017-01-01','2018-01-01']
	for i in range(0,11):
		start_time = datetime.date(datetime.strptime(version_date[i], '%Y-%m-%d'))
		end_time = datetime.date(datetime.strptime(version_date[i + 1], '%Y-%m-%d'))
		#m1=MAX_RW(start_time, end_time)	
		#m2=INTST_RW(start_time, end_time)
		#m3=self.ENT_RW(start_time, end_time)

		m4=self.LAT_RW(start_time, end_time)
		"""
		m5=self.OVERWORK(start_time, end_time)
		m6=self.CPLX_RR(start_time, end_time)
		m7=self.CPLX_F(start_time, end_time)
		m8=self.QLTY_ANR(start_time, end_time)
		m9=self.QLTY_RSRO(start_time, end_time)
		"""

