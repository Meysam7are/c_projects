#ifndef DB_TABLE_FILE_TEMPLATE_HEADER_FILE
#define DB_TABLE_FILE_TEMPLATE_HEADER_FILE
#pragma once

#include <string>
#include <format>
#include <filesystem>

#include "logger.h"
#include "multifile_io.h"
#include "time_conversions.h"
#include "db_concepts.h"
#include "db_index_id.h"
#include "db_index_lin.h"
#include "db_index_map.h"


namespace mz {
    namespace db {

        template <mz::db::EntryType T>
        struct indexed_record {
            mutable int64_t Index{ 0 };
            T Entry;
        };





        struct db_table_errors
        {
            // needs update, differentiate structural errors with regards to read and write, 
            // will out of bounds errors.

            union
            {
                struct
                {
                    uint32_t open : 1;
                    uint32_t IO : 1;
                    uint32_t Corrupted : 1;
                    uint32_t IndexOverflow : 1;
                    uint32_t seekg : 1;
                    uint32_t seekp : 1;
                    uint32_t read : 1;
                    uint32_t write : 1;

                };
                uint32_t value{ 1 };
            };

            std::string string() const noexcept {
                if (!value) { return "storage_errors: NoErrors\n"; }

                std::string Res{ std::format("storage_errors: ") };
                if (open) Res += "open,";
                if (IO) Res += "IO,";
                if (Corrupted) Res += "Corrupted,";
                if (IndexOverflow) Res += "IndexOverflow,";
                if (seekg) Res += "seekg,";
                if (seekp) Res += "seekp,";
                if (read) Res += "read,";
                if (write) Res += "write,";
                Res.back() = '\n';
                return Res;
            }

        };


        template <mz::db::EntryType T>
        class db_table_file
        {

        public:

            static constexpr size_t RecordSize{ sizeof(T) };

            using entry_type = T;
            using row_type = indexed_record<entry_type>;

            mz::io::file File;
            size_t MaxIndexes{ 0 };
            size_t NumIndexes{ 0 };

            mutable mz::db::db_table_errors Errors;
            mutable std::string ErrMsg{};

            std::string report_errors() const noexcept {
                return std::format("{}", File.eflags().value);
            }


            constexpr size_t row_offset(size_t Index) const noexcept { return RecordSize * Index; }
            constexpr int64_t count() const noexcept { return (int64_t)NumIndexes; }
            constexpr int64_t last_index() const noexcept { return int64_t(NumIndexes) - 1; }
            constexpr uint32_t nextIndex() const noexcept { return uint32_t(NumIndexes); }
            constexpr bool bad() const noexcept { return Errors.value || File.bad(); }
            constexpr bool fail() const noexcept { return Errors.value || File.fail(); }
            constexpr bool good() const noexcept { return !Errors.value && File.good(); }



            bool select(row_type& Row) const noexcept
            {
                if (seekg_index(Row.Index) || select_next(Row.Entry))
                {
                    mz::ErrLog << std::format("select_entry(,{}) fail", Row.Index);
                    Row.Index = -112;
                    return true;
                }
                return false;
            }

            bool update(row_type const& Row) noexcept
            {
                if (seekp_index(Row.Index) || update_next(Row.Entry))
                {
                    Row.Index = -113;
                    mz::ErrLog << std::format("update_entry(,{}) fail", Row.Index);
                    return true;
                }
                return false;
            }

            bool filter(row_type& Row, auto&& Func) noexcept
            {
                if (select(Row))
                {
                    mz::ErrLog << std::format("filter({}) select fail", Row.Index);
                    Row.Index = -123;
                    return true;
                }
                if (Func(Row)) {
                    return true;
                }

                if (update(Row)) {
                    mz::ErrLog << std::format("filter({}) update fail", Row.Index);
                    Row.Index = -125;
                    return true;
                }
                return false;
            }




            bool insert(row_type const& Row) noexcept
            {
                if (Row.Index != NumIndexes)
                {
                    mz::ErrLog << std::format("insert_entry(...) Index mismatch.  {}\n", mz::db::db_time::now().string());
                    Row.Index = -3;
                    return true;
                }

                if (!good())
                {
                    mz::ErrLog << std::format("insert_entry(...) not good.  {}\n", mz::db::db_time::now().string());
                    Errors.IO = 1;
                    Row.Index = -2;
                    return true;
                }

                if (NumIndexes >= MaxIndexes)
                {
                    mz::ErrLog << std::format("insert_entry(...) Index overflow.  {}\n", mz::db::db_time::now().string());
                    Errors.IndexOverflow = 1;
                    Row.Index = -3;
                    return true;
                }



                if (File.bseek_end(0))
                {
                    mz::ErrLog << std::format("insert_entry(...) seekp_end error.  {}\n", mz::db::db_time::now().string());
                    Errors.seekp = 1;
                    Row.Index = -4;
                    return true;
                }

                if (File.write(Row.Entry))
                {
                    mz::ErrLog << std::format("insert_entry(...) seekp_end error.  {}\n", mz::db::db_time::now().string());
                    Errors.write = 1;
                    Row.Index = -5;
                    return true;
                }
                Row.Index = static_cast<int64_t>(NumIndexes++);
                return false;
            }


            int64_t pop() noexcept
            {
                if (NumIndexes > 0) {
                    return int64_t(--NumIndexes);
                }
                else {
                    return -1;
                }
            }



            db_table_file() noexcept = default;

            bool create(std::wstring const& Path, size_t max_size) noexcept
            {
                //fmt::print("creating table_file\n");
                if (!File.excl(Path, O_BINARY))
                {
                    //fmt::print("failed to create\n");
                    return false;
                }
                return true;
            }


            int open(std::filesystem::path const& Name, size_t max_indexes) noexcept
            {
                NumIndexes = 0;
                MaxIndexes = 0;
                Errors.value = 0;
                std::string ErrMsg2 { std::format("storage[{}]::open: ", Name.string()) };
                if (Name.empty()) {
                    Errors.open = 1;
                    ErrMsg2 += "Name Empty\n";
                    mz::ErrLog << ErrMsg2;
                    return -1;
                }

                if (File.is_open()) {
                    File.close();
                    Errors.open = 1;
                    ErrMsg2 += "trying to reopen and open file\n";
                    mz::ErrLog << ErrMsg2;
                    return -3;
                }

                if (!max_indexes)
                {
                    File.close();
                    Errors.open = 1;
                    ErrMsg2 += "invalid maximum row numbers == 0\n";
                    mz::ErrLog << ErrMsg2;
                    return -4;
                }

                if (!File.create(Name, O_BINARY))
                {
                    File.close();
                    Errors.open = 1;
                    ErrMsg2 += std::format("failed to create/open file:\n{}", report_errors());
                    mz::ErrLog << ErrMsg2;
                    return -5;
                }

                auto L = File.size();
                if (File.fail() || L < 0) {
                    File.close();
                    Errors.IO = 1;
                    Errors.open = 1;
                    ErrMsg2 += std::format("File.fail() || L(={}) < 0:\n{}", L, report_errors());
                    mz::ErrLog << ErrMsg2;
                    return -6;
                }

                if (L % RecordSize)
                {
                    File.close();
                    Errors.open = 1;
                    Errors.Corrupted = 1;
                    ErrMsg2 += std::format("File.size(={}) % RecordSize(={}) = {} != 0\n", L, RecordSize, L % RecordSize);
                    mz::ErrLog << ErrMsg2;
                    return -7;
                }

                MaxIndexes = max_indexes;
                NumIndexes = L / RecordSize;
                if (NumIndexes >= MaxIndexes)
                {
                    File.close();
                    Errors.open = 1;
                    Errors.IndexOverflow = 1;
                    ErrMsg2 += std::format("NumIndex(={}) >= MaxIndexes(={})\n", NumIndexes, MaxIndexes);
                    mz::ErrLog << ErrMsg2;
                    return -8;
                }

                if (seekg_index(0))
                {
                    File.close();
                    Errors.open = 1;
                    ErrMsg2 += std::format("seekg_set\n{}", report_errors());
                    mz::ErrLog << ErrMsg2;
                    return -9;
                }

                if (seekp_index(count()))
                {
                    File.close();
                    Errors.open = 1;
                    ErrMsg2 += std::format("seekp_end\n{}", report_errors());
                    mz::ErrLog << ErrMsg2;
                    return -10;
                }

                ErrMsg2.clear();
                return 0;
            }



            bool generate_report(std::string& Report, auto&& Func) {
                Report += std::format("Number of Record : {}\n"
                    "------------------------------\n", count());

                if (!count()) {
                    Report += "No Records Exists.\n";
                    return false;
                }

                if (seekg_index(0)) {
                    Report += "Error Reading File.\n";
                    return true;
                }

                entry_type Entry;
                for (int64_t i = 0; i < count(); i++) {
                    if (!select_next(Entry)) {
                        Report += Func(Entry); //
                    }
                    else {
                        Report += "Error Reading File.\n";
                        return true;
                    }
                }
                return false;
            }











            bool select_next(T& Entry) const noexcept
            {
                if (File.read(Entry))
                {
                    mz::ErrLog << std::format("select_next() File.read fail");
                    Errors.read = 1;
                    return true;
                }

                return false;
            }

            bool update_next(T const& Entry) noexcept
            {
                if (File.write(Entry))
                {
                    mz::ErrLog << std::format("update_next() File.write fai");
                    Errors.write = 1;
                    return true;
                }

                return false;
            }

            bool filter_next(T& Entry, auto&& filter) noexcept
            {
                if (select_next(Entry)) { return true; }
                if (!filter(Entry)) { return false; }
                return update_next(Entry);
            }


            bool good(size_t Index) const noexcept
            {
                if (Index < NumIndexes)
                {
                    if (!Errors.value) {
                        return true;
                    }
                    else {
                        Errors.IO = 1;
                        mz::ErrLog << std::format("Pre-Existing Errors:{} while requesting good({})", Errors.value, Index);
                        return false;
                    }
                }
                else {
                    mz::ErrLog << std::format("good({}) index out bounds for NumIndexes = {}", Index, NumIndexes);
                    return false;
                }
            }


            bool seekg_index(int64_t Index) const noexcept
            {
                if (!good(Index))
                {
                    mz::ErrLog << std::format("seekg_index({}) not good", Index);
                    return true;
                }

                if (File.bseek_set(row_offset(Index)))
                {
                    mz::ErrLog << std::format("seekg_index({}) File.seekg_set fail", Index);
                    
                    Errors.seekg = 1;
                    return true;
                }

                return false;
            }


            bool seekp_index(int64_t Index) noexcept
            {
                if (!good(Index))
                {
                    mz::ErrLog << std::format("seekp_index({}) not good", Index);
                    return true;
                }

                if (File.bseek_set(row_offset(Index)))
                {
                    mz::ErrLog <<std::format("seekp_index({}) File.seek_set fail", Index);
                    
                    Errors.seekp = 1;
                    return true;
                }

                return false;
            }




        };



    }
};





#endif


