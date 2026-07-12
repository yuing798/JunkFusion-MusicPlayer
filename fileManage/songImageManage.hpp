
#include "constants.h"
#include <SQLiteCpp/Database.h>
#include <memory>
class SongImageManager{
private:
    std::unique_ptr<SQLite::Database> db;
    void createTable();
public:
    SongImageManager();
    static SongImageManager& getInstance(){
        static SongImageManager instance;
        return instance;
    }
    DONT_COPY_AND_MOVE(SongImageManager)
};
