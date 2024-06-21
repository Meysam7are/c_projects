#ifndef DB_TABLE_HEADER_FILE
#define DB_TABLE_HEADER_FILE
#pragma once

#include "db_index_id.h"
#include "db_index_lin.h"
#include "db_index_map.h"
#include "db_table_file.h"

namespace mz {
	namespace db {


        template <mz::db::EntryType E, template<mz::db::KeyType> typename T>
        class db_table {


        public:
            using entry_type = E;
            using row_type = mz::db::indexed_record<entry_type>;

            using key_type = typename entry_type::key_type;
            using map_type = T<key_type>;
            using value_type = typename map_type::value_type;
            using keyval_ref = typename map_type::keyval_ref;

            using pk_iterator = typename map_type::iterator;
            using pk_const_iterator = typename map_type::const_iterator;
            using insert_return_type = typename map_type::insert_return_type;


            map_type keys;
            mz::db::db_table_file<entry_type> storage;
            std::string const Name;





            db_table(std::string const& Name) : Name{ Name } {}

            ~db_table() {  }


            pk_iterator select_key(keyval_ref KV) noexcept { return keys.select(KV); }
            pk_iterator select_key(row_type& Row) noexcept { return keys.select(keyval_ref{ Row.Entry.pk(), Row.Index }); }


            bool update(row_type& Row)
            {
                auto it = select_key(Row);
                if (it == keys.end()) {
                    mz::ErrLog << std::format("db_table[{}]::update({}) not found\n", Name, Row.Entry.pk().string());
                    return true;
                }

                if (storage.update(Row))
                {
                    Row.Index = -2;
                    mz::ErrLog << std::format("db_table[{}]::update({}) corrupted\n", Name, Row.Entry.pk().string());
                    return true;
                }
                else {
                    //key(it) = Row.Entry.pk();
                    return false;
                }
            }






            bool remove(row_type& Row)
            {
                auto it = select_key(Row);
                if (it == keys.end()) {
                    mz::ErrLog << std::format("db_table[{}]::remove({}) not found\n", Name, Row.Entry.pk().string());
                    Row.Index = -1;
                    return true;
                }

                if (Row.Index + 1 == storage.count())
                {
                    storage.pop();
                    keys.erase(it);
                    return false;
                }

                Row.Entry.erase();
                if (storage.update(Row))
                {
                    mz::ErrLog << std::format("db_table[{}]::rempve({}) corrupted\n", Name, Row.Entry.pk().string());
                    Row.Index = -2;
                    return true;
                }

                keys.erase(it);
                return false;
            }





            bool select(row_type& Row)
            {
                auto it = select_key(Row);
                if (it == keys.end()) {
                    mz::ErrLog << std::format("db_table[{}]::select({}) not found\n", Name, Row.Entry.pk().string());
                    Row.Index = -1;
                    return true;
                }

                key_type Key{ Row.Entry.pk() };
                if (storage.select(Row)) {
                    mz::ErrLog << std::format("db_table[{}]::select({}) corrupted\n", Name, Row.Entry.pk().string());
                    Row.Index = -2;
                    return true;
                }

                if (Key != Row.Entry.pk()) {
                    mz::ErrLog << std::format("db_table[{}]::select({}) updated\n", Name, Row.Entry.pk().string());
                }
                return false;
            }






            int load(std::filesystem::path const& Folder, auto&& Func)
            {
                if (int Res = open(Folder); Res) { return Res; }

                row_type Row;
                keys.clear();
                keys.reserve(storage.count());
                //DataMsg = std::format("db_table[{}]::load: ", Name);

                for (Row.Index = 0; Row.Index < storage.count(); Row.Index++)
                {
                    if (storage.select_next(Row.Entry))
                    {
                        mz::ErrLog << std::format("db_table[{}]::load:storage::select_next({}) file error: {}\n", Name, Row.Index, storage.File.eflags().value);
                        //DataMsg += 
                        //fmt::print("{}\n", DataMsg);
                        return 5000;
                    }

                    int Res = Func(Row);
                    if (Res) {
                        //fmt::print("{}\n", DataMsg);
                        return Res;
                    }
                }

                //DataMsg.clear();
                return 0;
            }



            int load(std::filesystem::path const& Folder)
            {
                auto Inserter = [&](row_type& Row) noexcept -> int
                    {
                        if (!keys.insert(Row.Entry.pk(), Row.Index).second)
                        {
                            mz::ErrLog << std::format("db_table[{}]::load:index_map::insert({}) duplicate.\n", Name, Row.Index);
                            return 6000;
                        }
                        else { return 0; }
                    };

                return load(Folder, std::forward<decltype(Inserter)>(Inserter));
            }









            int open(std::filesystem::path const& Folder)
            {
                //DataMsg.clear();
                if (int Res = storage.open(Folder / Name, 1000000ULL); Res)
                {
                    //DataMsg = std::format("db_table[{}]::open: storage.open return with errors\n{}", Name, storage.ErrMsg);
                    //fmt::print("{}\n", DataMsg);
                    return Res;
                }
                return 0;
            }



        protected:



            bool insert(row_type& Row)
            {
                Row.Index = storage.count();
                auto [it, success] = keys.insert(Row.Entry.pk(), Row.Index);
                if (!success) {
                    mz::ErrLog << std::format("db_table::insert({}) failed\n", Row.Entry.pk().string());
                    Row.Index = -1;
                    return true;
                }
                if (storage.insert(Row))
                {
                    mz::ErrLog << std::format("db_table::insert({}) corrupted\n", Row.Entry.pk().string());
                    keys.pop(it);
                    Row.Index = -2;
                    return true;
                }

                return false;
            }


        };


    }




}

#endif
