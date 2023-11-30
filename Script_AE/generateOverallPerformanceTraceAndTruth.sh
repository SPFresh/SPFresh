setname="6c VectorPath=spacev100m_update_set"
truthname="18c TruthPath=spacev100m_update_truth"
deletesetnameprefix="spacev100m_update_set"
for i in {0..99}
do
    /home/sosp/SPFresh/Release/usefultool -GenTrace true --vectortype int8 --VectorPath /home/sosp/data/spacev_data/spacev200m_base.i8bin --filetype DEFAULT --UpdateSize 1000000 --BaseNum 100000000 --ReserveNum 100000000 --CurrentListFileName spacev100m_update_current --ReserveListFileName spacev100m_update_reserve --TraceFileName spacev100m_update_trace -NewDataSetFileName spacev100m_update_set -d 100 --Batch $i -f DEFAULT
    newsetname=$setname$i
    deletesetname=$deletesetnameprefix$i
    sed -i "$newsetname" genTruth.ini
    sed -i "$newtruthname" genTruth.ini
    /home/sosp/SPFresh/Release/ssdserving genTruth.ini
    /home/sosp/SPFresh/Release/usefultool -ConvertTruth true --vectortype int8 --VectorPath /home/sosp/data/spacev_data/spacev200m_base.i8bin --filetype DEFAULT --UpdateSize 1000000 --BaseNum 100000000 --ReserveNum 100000000 --CurrentListFileName spacev100m_update_current --ReserveListFileName spacev100m_update_reserve --TraceFileName spacev100m_update_trace -NewDataSetFileName spacev100m_update_set -d 100 --Batch $i -f DEFAULT
    rm -rf $deletesetname
done