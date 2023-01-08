#include <iostream>
#include<fstream>
#include <vector>
#include <algorithm>
#include<iomanip>
#include<stack>
using namespace std;
int M = 5, enterLeft = 0;
// recod class to represent a page
class Record {
public:
    int leaf;
    vector<pair<int, int>>node;
    Record(int m = M) {
        M = m;
        this->leaf = -1;
        this->node = vector<pair<int, int>>(m, { -1,-1 });
    }
    static int size() 
    {
        
        return 4 + (M * 2 * 4);
    }
    int len() {
        int free = 0;
        for (size_t i = 0; i < M; i++)
        {
            if (node[i].first == -1 || node[i].second == -1)
                free++;
        }
        return M - free;
    }

    bool isLeaf() {
        return !leaf;
    }
    friend std::ostream& operator<<(std::ostream& str, Record const& data)
    {
        str << data.leaf << "\t";
        for (auto i : data.node) {
            str << i.first << "\t" <<  i.second << "\t";
        }
        return str;
    }
    friend std::istream& operator>>(std::istream& str, Record& data)
    {
        str >> data.leaf;
        for (int i = 0; i < data.node.size(); i++)
        {
            str >> data.node[i].first >> data.node[i].second;
        }
        return str;
    }
};
// prototype for insertInLeaf
int insertInLeaf(fstream& file, int recByteOffset, int grandByteOffset, int key, int value);
// search

// compare for sorting 
bool compare(pair<int, int> p1, pair<int, int>p2) {
    if (p1.first == -1)
        return false;
    return p1.first < p2.first;
}
void sortRec(Record& root,int len) {
    sort(root.node.begin(), root.node.begin() + len , compare);

}
// write record on the file
void writeRecord(fstream& file, Record& record,int recByteOffset) {
    if (file.tellp() != recByteOffset) {   
        file.seekp(recByteOffset, ios::beg);
    }
    file.write(reinterpret_cast<const char*>(&record.leaf), sizeof(record.leaf));
    file.write(reinterpret_cast<const char*>(&record.node[0]), record.node.size() * sizeof(pair<int,int>));
}
//  read record from the file
Record readRecord(fstream& file, Record& record ,int recByteOffset) {
    if (file.tellg() != recByteOffset) {
        file.seekg(recByteOffset, ios::beg);
    }
    file.read(( char*)(&record.leaf), sizeof(record.leaf));
    file.read((char*)(&record.node[0]), record.node.size() * sizeof(pair<int, int>));
    return record;
}
// write header
void writeHeader(fstream&file,int value) {
    int start = 4;
    file.seekp(start, ios::beg);
    file.write((char*)&value, sizeof(int));

}
// read header
int readHeader(fstream& file) {
    int start = 4;
    file.seekg(start, ios::beg);
    int value;
    file.read((char*)&value, sizeof(int));
    return value;
}
//create index file initialize it with -1
void createIndexFIle(string fileName, int numOfRecords, int m) {
    fstream file;
    file.open(fileName, ios::out | ios::binary);
    if (!file) { cout << "can't find file\n"; return; }
    Record header(m);
    header.node[0].first = 1;
    writeRecord(file, header, 0);
    for (int i = 1; i < numOfRecords; i++)
    {
        Record rec(m);
        if (i != numOfRecords - 1)
            rec.node[0].first = (i + 1);
        writeRecord(file, rec,file.tellp());
    }
    file.close();
}
//display the table on the console
void displayTable(string fileName, int numOfRecords, int m) {
    fstream file;
    file.open(fileName, ios::in | ios::binary);
    if (!file) { cout << "can't find file\n"; return; }
    cout << "\n";
    for (size_t i = 0; i < numOfRecords; i++)
    {
        Record rec(m);
        readRecord(file, rec,file.tellg());
        cout<< rec << " \n";
    }
    cout << "\t\t----------------------------------------------------------\n";
    file.close();
}

// search for insertion
int searchForInsertion(fstream&file, int key, int pos) {
    Record record(M);
    // skip header
    readRecord(file, record, record.size() * pos);
    if (record.isLeaf()) {
        for (size_t i = 0; i < M; i++)
            if (record.node[i].first == key) 
                return record.node[i].second;
        return -1;
    }
    else {
        for (size_t i = 0; i < M; i++) {
            if (record.node[i].first >= key)
                return searchForInsertion(file, key, record.node[i].second);

        }
        return -1;
    }
}
// search interface
bool isFound(string fileName, int key) {
    fstream file;
    file.open(fileName, ios::in | ios::out | ios::binary);
    if (!file) { cout << "file failure"; return 0; }
    if (searchForInsertion(file, key, 1) != -1) {
        file.close();
        return true;
    }
    else {
        file.close();
        return false;
    }
}
// search for value
int searchForValue(string fileName, int key) {
    fstream file;
    file.open(fileName, ios::in | ios::out | ios::binary);
    if (!file) { cout << "file failure"; return 0; }
    return searchForInsertion(file, key, 1);

}
// rebuild the record when split
int handleParent(fstream& file, Record& parent, Record& left, Record& right, int lPos, int rPos, int parentByteOffset) {
    parent.leaf = 1;
    parent.node = vector < pair<int, int>>(M, { -1,-1 });
    int leftLen = left.len();
    int rightLen = right.len();
    // get the nodes to up to the parent
    int leftVolantary = left.node[leftLen - 1].first;
    int rightVolantary = right.node[rightLen - 1].first;
    if (leftVolantary < rightVolantary) {
        parent.node[0] = { leftVolantary,lPos };
        parent.node[1] = { rightVolantary,rPos };

    }
    else {
        parent.node[1] = { leftVolantary,lPos };
        parent.node[0] = { rightVolantary,rPos };
    }
    writeRecord(file, parent, parentByteOffset);
    return max(leftVolantary, rightVolantary);
}
//split
int splitInNonLeaf(fstream& file, Record& parent, int parentByteOffset, int key, int value) {
    int leftPos = readHeader(file);
    if (leftPos == -1) {
        return -1;
    }
    Record left, right;
    int leftput = 0, rightput = 0;
    int mid = ((M + 1) / 2);
    for (size_t i = 0; i < M; i++)// in left 
    {
        if (i < mid) {
            left.node[i] = parent.node[i];
            leftput++;
        }
        //else if (i == M / 2) {
        //    left.node[i - 1].first < key;     // 1 5        3
        //    left.node[i] = parent.node[i];
        //    leftput++;
        //}
        else {
            right.node[rightput] = parent.node[i];
            rightput++;
        }
    }
    // insert the node
    if (parent.node[mid - 1].first >= key) {
        left.node[leftput] = { key,value };
        sortRec(left, leftput + 1);
    }
    else {
        right.node[rightput] = { key,value };
        sortRec(right, rightput + 1);
    }
    file.seekg(leftPos, ios::beg);
    Record temp;
    readRecord(file, temp, leftPos * temp.size());// 1 3 5           4           // 7 9 10
    int rightPos = temp.node[0].first;
    if (rightPos == -1) {
        return -1;
    }
    file.seekg(leftPos, ios::beg);
    // write the left Record
    left.leaf = parent.leaf;
    writeRecord(file, left, leftPos * temp.size());
    Record temp2;
    readRecord(file, temp2, rightPos * temp.size());
    //update the original header
    writeHeader(file, temp2.node[0].first);
    right.leaf = parent.leaf;
    // wirte the right Record
    writeRecord(file, right, rightPos * temp.size());
    // upadate parent Record
    return handleParent(file, parent, left, right, leftPos, rightPos, parentByteOffset);

}
// update node index 
void updateInRecord(Record& rec,int oldValue, int newValue) {
    for (size_t i = 0; i < rec.node.size(); i++)
    {
        if (rec.node[i].first == oldValue)
        {
            rec.node[i].first = newValue;
            return;
        }
    }
}

//split
int splitInLeaf(fstream& file, Record& source, int sourceByteOffset,int grandPos, int key,int value) {
    int leftPos = readHeader(file);
    if (leftPos == -1) {
        cout << "no available places\n";
        return -1;
    }
    Record left, right;
    int leftput = 0, rightput = 0;
    int mid = (M + 1) / 2;
    for (size_t i = 0; i < M; i++)// in left 
    {
        if (i < mid) {
            left.node[i] = source.node[i];
            leftput++;
        }
        else {
            right.node[rightput] = source.node[i];
            rightput++;
        }
    }
    // insert the node
    int mOldRight = right.node[rightput - 1].first;
    if (source.node[mid - 1].first >= key) {
        left.node[leftput] = { key,value };
        sortRec(left, leftput + 1);
    }
    else {
        right.node[rightput] = { key,value };
        sortRec(right, rightput + 1);
    }
   // update header
    Record temp;
    readRecord(file, temp, leftPos * Record::size());
    writeHeader(file, temp.node[0].first);
    // write the left Record
    left.leaf = source.leaf;
    right.leaf = source.leaf;
    // right left and right records
    writeRecord(file, left, leftPos * Record::size());
    writeRecord(file, right, sourceByteOffset);
    // update max right in parent
    int rightLen = right.len();
    int rightVolantary = right.node[rightLen - 1].first;
    if (mOldRight < rightVolantary) {
        readRecord(file, temp, grandPos * Record::size());
        updateInRecord(temp, mOldRight, rightVolantary);
        writeRecord(file, temp, grandPos * Record::size());
    }
    // inset  a pointer in parent to point left 
    int leftLen = left.len();
    int leftVolantary = left.node[leftLen - 1].first;
     insertInLeaf(file, grandPos*Record::size(),( grandPos -1)* Record::size(),leftVolantary, leftPos);
    
}
// insert in leaf and return the max element in the new record even if split happened
int insertInLeaf(fstream& file, int recByteOffset,int grandPos,int key, int value) {//////
    if (file.tellg() != recByteOffset)
        file.seekg(recByteOffset, ios::beg);
    Record rec;
    readRecord(file, rec, recByteOffset);
    int recLen = rec.len();
    if (recLen < M) {
        //rec.leaf = 0;////////////////////
        rec.node[recLen] = { key,value };
        sortRec(rec, recLen+1);
        int maxEl = rec.node[recLen].first;
        writeRecord(file, rec, recByteOffset);
        return maxEl;
    }
    else {
        if (recByteOffset == Record::size() || !rec.isLeaf() || grandPos == -1) {
             splitInNonLeaf(file, rec, recByteOffset, key, value);
        }
        else {
              splitInLeaf(file, rec, recByteOffset,grandPos, key, value);
        }
    }

}
//search for insertion
Record findSpecificLeaf(fstream& file, int parentPos, int key) {
    Record parent;
    readRecord(file, parent, parentPos * parent.size());
    if (parent.isLeaf()) {
        /// <summary>
        /// /
        /// </summary>
        /// <param name="file"></param>
        /// <param name="parentPos"></param>
        /// <param name="key"></param>
        /// <returns></returns>
        return parent;
    }
    else {
        int len = parent.len();
        for (size_t i = 0; i < len; i++) //   10 1       15  2    (50)
        {
            if (parent.node[i].first >= key) {
                return findSpecificLeaf(file, parent.node[i].second, key);
            }
            else if (i == len - 1) {
                parent.node[i].first = key;
                return findSpecificLeaf(file, parent.node[i].second, key);
            }
        }
    }
    
}
//insert in non leaf record
int insertNonLeaf(fstream&file, int recPos, int parentPos, int key, int value) {
    Record parent;
    readRecord(file, parent, recPos * parent.size());
    if (parent.isLeaf()) {
       return insertInLeaf(file, recPos * Record::size(), parentPos,key, value);
    }
    else {
        int len = parent.len();
        int maxInNonLeaf = -2;
        for (size_t i = 0; i < len; i++) //   10 1       15  2    (50)
        {
            if (parent.node[i].first >= key) {
                maxInNonLeaf = insertNonLeaf(file, parent.node[i].second,recPos, key, value);
                // to hundle incase no free places
                if (maxInNonLeaf == -1) {
                    return -1;
                }
                if (maxInNonLeaf > parent.node[i].first) {
                    parent.node[i].first = maxInNonLeaf;
                    writeRecord(file, parent, recPos * Record::size());
                }
                return parent.node[i].first;
            }
            else if (i == len - 1) {
                maxInNonLeaf = insertNonLeaf(file, parent.node[i].second, recPos, key, value);
                // to hundle incase no free places
                if (maxInNonLeaf == -1) {
                    return -1;
                }
                if (maxInNonLeaf > parent.node[i].first) {
                    parent.node[i].first = maxInNonLeaf;
                    writeRecord(file, parent, recPos * Record::size());
                }
                return parent.node[i].first;
            }
        }
     
    }
}

// insert node in the table
bool insert(string fileName, int key, int value) {
    fstream file;
    file.open(fileName, ios::in |ios::out | ios::binary);   // 3 12 ------  
    if (!file) { cout << "file failure"; return 0; }
    int nextFree = readHeader(file);
   // if(nextFree == -1){}
    if (nextFree == 1) {
        Record root;
        readRecord(file, root, root.size() * nextFree);
        // update header
        writeHeader(file, root.node[0].first);
        root.leaf = 0;
        writeRecord(file, root, 1 * Record::size());
        insertInLeaf(file, 1 * Record::size(),-1, key, value);
    }
    else {
        //Record root;
        //readRecord(file, root, root.size() * 1);
        //if (root.isLeaf()) {
        //    /*int length = root.len();
        //    if(length!=M){
        //        root.node[length].first = key;
        //        root.node[length].second = value;
        //        sortRec(root, length + 1);
        //        writeRecord(file, root, root.size() * 1);*/
        //    insertInLeaf(file, 1 * Record::size(), key, value);
        //    }
        //else {
        //}
        //}
        if (insertNonLeaf(file, 1, -1, key, value) == -1) {
            file.close();
            return false;
        }
    }
    file.close();
    // write the new header
    return 1;
}

// deleteNodeFromRec
void deleteNodeFromRec(Record& rec, int key) {
    for (size_t i = 0; i < rec.node.size(); i++)
    {
        if (rec.node[i].first == key) {
            rec.node[i] = make_pair(-1, -1);
            break;
        }
    }
    for (size_t i = 0; i < rec.node.size() - 1; i++)
    {
        if (rec.node[i].first == -1) {
            swap(rec.node[i], rec.node[i + 1]);
        }
    }

}
// right borrow
int rightSibBorrow(fstream& file, Record& parent, Record& record,int recPos, int oldMax) {
    for (size_t i = 0; i < parent.node.size() - 1; i++)
    {
        if (parent.node[i].first == -1) {
            return -1;
        }
        else if (parent.node[i].first == oldMax && parent.node[i + 1].first != -1) {
            enterLeft = 0;
            Record sibling;
            readRecord(file, sibling, parent.node[i + 1].second * Record::size());
            if (sibling.len() > M / 2) {
                pair<int, int> sibNode = sibling.node[0];
                deleteNodeFromRec(sibling, sibNode.first);
                writeRecord(file, sibling, parent.node[i + 1].second * Record::size());
                // udate record
                record.node[M / 2 -1] = sibNode;
                writeRecord(file, record, recPos * Record::size());
                parent.node[i].first= sibNode.first;
                return sibNode.first;
            }
            else 
                return -1;
        }
    }
    return -1;
}
// left borrow
int leftSibBorrow(fstream& file, Record& parent, Record& record,int parentPos, int recPos, int oldMax) {
    for (size_t i = 1; i < parent.node.size(); i++)
    {
        if (parent.node[i].first == -1) {
            return -1;
        }
        else if (parent.node[i].first == oldMax && parent.node[i - 1].first != -1) {
            enterLeft = 1;
            Record sibling;
            readRecord(file, sibling, parent.node[i - 1].second * Record::size());
            if (sibling.len() > M / 2) {
                int len = sibling.len();
                pair<int, int> sibNode = sibling.node[len - 1];
                deleteNodeFromRec(sibling, sibNode.first);
                writeRecord(file, sibling, parent.node[i - 1].second * Record::size());
                // udate record
                record.node[M / 2 - 1] = sibNode;
                int recSize = record.len();
                sortRec(record, recSize);
                writeRecord(file, record, recPos * Record::size());
                // handle parent
                parent.node[i - 1].first = sibling.node[len - 2].first;
                writeRecord(file, parent, parentPos * Record::size());
                return record.node[ recSize- 1].first;
            }
            else
                return -1;
        }
    }
    return -1;
}
// merge to records
void mergeRecord(Record& rec1, Record& rec2) {
    int start = rec1.len();
    int i;
    for ( i = start; i < rec1.node.size(); i++)
    {
        if (rec2.node[i - start].first == -1)
            break;

        rec1.node[i] = rec2.node[i - start];
        rec2.node[i] = make_pair(-1, -1);
    }
    sortRec(rec1, i);
}
//get parent refference
int parentRefference(Record&parent ,int key) {
    for (size_t i = 0; i < parent.node.size() - 1; i++)
    {
        if (parent.node[i].first == key)
            return i;
    }
}
// merge in delete
void mergeDeleted(fstream& file, Record& parent,  int parentPos,int oldMax,int mergeLeft) {
    int i = parentRefference(parent, oldMax);
    Record record;
    readRecord(file, record, parent.node[i].second * Record::size());
    Record sibling;
    if (mergeLeft == 0) {
        readRecord(file, sibling, parent.node[i +1].second * Record::size());
        mergeRecord(sibling, record);
        int sibSize = sibling.len();
        writeRecord(file, sibling, parent.node[i+1].second * Record::size());
        // update parent refference
        //parent.node[i +1].first = sibling.node[sibSize - 1].first;
         //update header
        int header = readHeader(file);
        record = Record(M);
        record.node[0].first = header;
        writeHeader(file, parent.node[i].second);

        writeRecord(file, record, parent.node[i].second * Record::size());
        //
    }
    else {
        deleteNodeFromRec(record, oldMax);
        readRecord(file, sibling, parent.node[i -1].second * Record::size());
        mergeRecord(sibling, record);
        int sibSize = sibling.len();
        writeRecord(file, sibling, parent.node[i - 1].second * Record::size());
            // update parent refference
        parent.node[i - 1].first = sibling.node[sibSize - 1].first;
        int header = readHeader(file);
        record = Record(M);
        record.node[0].first = header;
        writeHeader(file, parent.node[i].second);
        writeRecord(file, record, parent.node[i].second * Record::size());


    }
    deleteNodeFromRec(parent, oldMax);
    writeRecord(file, parent, parentPos * Record::size());

}
//delete
int deleteNode(fstream& file, int key,int recPos,int parentPos) {
    Record record(M);
    readRecord(file, record, record.size() * recPos);
    if (record.isLeaf()) {
        if (recPos == 1) {
            deleteNodeFromRec(record, key);
            writeRecord(file, record, recPos * Record::size());
            return -1;
        }
        int recSize = record.len();
        int oldMax = record.node[recSize - 1].first;
        deleteNodeFromRec(record, key);
  
        //get size
        if (recSize > M / 2 || recPos == 1) {
            writeRecord(file, record, recPos * Record::size());
            if (oldMax > record.node[recSize - 2].first) {
                return record.node[recSize - 2].first;
            }
            else
                return -1;
        }
        else {
            Record parent;
            readRecord(file, parent, parentPos * Record::size());
            int ret = rightSibBorrow(file, parent, record, recPos, oldMax);
            if (ret != -1) {
                writeRecord( file, parent, parentPos * Record::size());
                return ret;
            }
            else {
                int leftRet = leftSibBorrow(file, parent, record, parentPos, recPos, oldMax);
                if (leftRet != -1) {
                    writeRecord(file, parent, parentPos * Record::size());
                    return leftRet;
                }
                else {
                    writeRecord(file, record, recPos * Record::size());
                    mergeDeleted(file, parent, parentPos, oldMax,enterLeft);
                    return -1;
                }
            }

        }
    }
    else {
        for (size_t i = 0; i < M; i++) { // 5             3  5            20  
            if (record.node[i].first >= key) {
                int ret = deleteNode(file, key, record.node[i].second, recPos);
                if (ret != -1 && ret < record.node[i].first) {
                    readRecord(file, record, recPos * Record::size());
                    record.node[i].first = ret;
                    writeRecord(file, record, recPos*Record::size());
                    return ret;
                }
                return -1;
            }
            

        }
    }
}
// delete interface
bool deleteNode(string fileName, int key) {
    if (!isFound(fileName, key)) {
        cout << "not exist\n";
        return 0;
    }
    else {
        fstream file;
        file.open(fileName, ios::in | ios::out | ios::binary);   // 3 12 ------  
        if (!file) { cout << "file failure"; return 0; }
        deleteNode(file, key, 1, -1);
        return 1;
    }
    
}

int main()
{
    string fileName = "file.bin";
    int numOfRecords;
    int m;
    int test = 40;

    while (test--)
    {
        cout << "1: inset \t\t\t 2: delete\t\t 3: search\t\t4.create\n";
        int x; cin >> x;
        if (x == 4) {
            numOfRecords = 5;
            m = 4;
            createIndexFIle(fileName, numOfRecords, m);
            displayTable(fileName, numOfRecords, m);
        }

            if (x == 1) {
            //  1-insert
            cout << "enter key and value : ";
            int a, b; cin >> a >> b;
            if (isFound(fileName, a)) {
                cout << "this key is alreary exist\n\n";
            }
            else {
                insert(fileName, a, b);
                displayTable(fileName, numOfRecords, m);
            }
        }
        else if(x==2){
            // delete
            cout << "enter key: ";
            int a; cin >> a;
            deleteNode(fileName, a);
            displayTable(fileName, numOfRecords, m);
        }
        else if (x == 3) {
            //search
            cout << "enter key: ";
            int a; cin >> a;
            cout <<"the value is: " << searchForValue(fileName, a) << "\n";
        }
    }
    // 1 3 12 1  7   24  1 10  48 1 24 60   1 14 72 1 19 84  1  30 96  1 15 108  1  1 120 1  5 133 1  2 144
    //   1 8 156   1 9 168    1 6 180  1 11 192   1  12 204  1   17 216   1 18 228 1  32 240
    // 2 10 2 9 2 8
    //  2 14 2 12 2 10 2 4 2 32 2 5 2 3 2 19 2 11 2 2 2 8
    // 
    //1 3 12 1  7   24  1 10  48 1 24 60   1 14 72 1 19 84  1  30 96  1 15 108  2 10 2 30 2 14 2 24
    // 1 3 12 1  7   24  1 10  48 1 24 60   1 14 72 1 19 84  1  30 96 2 30 2 19 2 24 


}


