/*
This file is part of ezOptionParser. See MIT-LICENSE.
Copyright (C) 2011,2012,2014 Remik Ziemlinski <first d0t surname att gmail>
CHANGELOG
v0.0.0 20110505 rsz Created.
v0.1.0 20111006 rsz Added validator.
v0.1.1 20111012 rsz Fixed validation of ulonglong.
v0.1.2 20111126 rsz Allow flag names start with alphanumeric (previously, flag had to start with alpha).
v0.1.3 20120108 rsz Created work-around for unique id generation with IDGenerator that avoids retarded c++ translation unit linker errors with single-header static variables. Forced inline on all methods to please retard compiler and avoid multiple def errors.
v0.1.4 20120629 Enforced MIT license on all files.
v0.2.0 20121120 Added parseIndex to OptionGroup.
v0.2.1 20130506 Allow disabling doublespace of OPTIONS usage descriptions.
v0.2.2 20140504 Jose Santiago added compiler warning fixes.
Bruce Shankle added a crash fix in description printing.
v0.2.3 20150403 Remove ezOptionValidator,set validate function in OptionGroup.
Support vc6,Clipping volume
v0.2.4 20151108 Add directory type Support
Add some annotation
v0.2.5 20151115 Change option parse function
v0.2.6 20151117 XorGroup implement
Format and check code
v0.2.7 20160803 Edit get function ,to get the space in the string(file path)
*/
#ifndef EZ_OPTION_PARSER_H
#define EZ_OPTION_PARSER_H

/* disable warnings  */
#if defined _MSC_VER &&  _MSC_VER<1400
#pragma warning(disable: 4786)
#endif
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <limits>
#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
namespace ez
{
    enum EZ_TYPE { EZ_NOTYPE = 0, EZ_BOOL, EZ_INT8, EZ_UINT8, EZ_INT16, EZ_UINT16, EZ_INT32, EZ_UINT32, EZ_INT64, EZ_UINT64, EZ_FLOAT, EZ_DOUBLE, EZ_TEXT, EZ_FILE, EZ_DIR };
    static const std::string EZ_TYPE_NAME[] = { "NOTYPE", "bool", "char", "unsigned char", "short", "unsigned short", "int", "unsigned int", "long", "unsigned long", "float", "double", "string", "file", "directory" };
    static const char delim = ',';
    /**
    * @brief 拆分字符串
    * @param s 输入字串
    * @param token 拆分标记
    * @param result 输出结果
    */
    static void SplitDelim(const std::string &s, const char token, \
        std::vector<std::string> &result)
    {
        std::stringstream ss(s);
        std::string item;

        while (std::getline(ss, item, token)) {
            result.push_back(item);
        }
    };
    /**
    * @brief 检查文件是否存在
    * @param name 文件名
    */
    static bool CheckFileExistence(const std::string &name)
    {
        struct stat buffer;
        return (stat(name.c_str(), &buffer) == 0);
    };
    /**
    * @brief 检查文件是否存在
    * @param name 文件名
    */
    static bool CheckDirExistence(const char *path)
    {
        struct stat info;

        if (stat(path, &info) != 0) {
            return false;
        }
        else if (info.st_mode & S_IFDIR) {
            return true;
        }
        else {
            return false;
        }
    };
    /**
    * @brief 参数类
    */
    class OptionGroup {
    public:
        OptionGroup() : expectArgs(0), \
            isRequired(false), \
            isSet(false), \
            needValidate(false), \
            optType(EZ_NOTYPE), \
            isUnlabeled(false), \
            groupID(0) { }
        ~OptionGroup() {};
        std::string defaults;  /** @brief 默认值  */
        int expectArgs;  /** @brief 期望参数个数  */
        std::string help;  /** @brief 参数说明  */
        std::string argsFormat;  /** @brief 参数格式  */
        bool isRequired;  /** @brief 是否必须  */
        std::vector<std::string> flags;  /** @brief 参数标识  */
        bool isSet;  /** @brief 是否设置  */
        bool needValidate;  /** @brief 需要验证  */
        std::vector<std::vector<std::string> > args;  /** @brief 参数列表  */
        std::vector<std::string> validValues;  /** @brief 允许值  */
        std::string maxValue;  /** @brief 最大值  */
        std::string minValue;  /** @brief 最小值  */
        EZ_TYPE optType;  /** @brief 参数类型  */
        bool isUnlabeled;  /** @brief 是否为无标签参数(如input,output等不带前导符的参数)  */
        int groupID;
        /**
        * @brief 获取参数
        * @param out 输出参数值
        */
        inline void get(std::string &out)
        {
            std::stringstream ss;

            if (!isSet) {
                if (defaults.empty()) {
                    ss << "";
                    ss >> out;
                }
                else {
                    ss << defaults;
                    ss >> out;
                }
            }
            else {
                if (args.empty() || args[0].empty()) {
                    ss << "";
                    ss >> out;
                }
                else {
                    ss << args[0].at(0);
                    out = ss.str();
                }
            }
        };
        /**
        * @brief 获取参数
        * @param out 输出参数值
        */
        template <typename T>
        inline void get(T &out)
        {
            std::stringstream ss;

            if (!isSet) {
                if (defaults.empty()) {
                    ss << "";
                    ss >> out;
                }
                else {
                    ss << defaults;
                    ss >> out;
                }
            }
            else {
                if (args.empty() || args[0].empty()) {
                    ss << "";
                    ss >> out;
                }
                else {
                    ss << args[0].at(0);
                    ss >> out;
                }
            }
        };
        /**
        * @brief 获取参数列表
        * @param out 输出参数列表
        */
        template <typename T>
        inline void getVector(std::vector<T> &out)
        {
            std::stringstream ss;

            if (!isSet) {
                if (!defaults.empty()) {
                    std::vector< std::string > strings;
                    SplitDelim(defaults, delim, strings);

                    for (int i = 0; i < (long int)strings.size(); ++i) {
                        ss << strings[i];
                        T temp;
                        ss >> temp;
                        out.push_back(temp);
                        ss.str("");
                        ss.clear();
                    }
                }
            }
            else {
                if (!(args.empty() || args[0].empty())) {
                    for (int i = 0; i < (long int)args[0].size(); ++i) {
                        ss << args[0].at(i);
                        T temp;
                        ss >> temp;
                        out.push_back(temp);
                        ss.str("");
                        ss.clear();
                    }
                }
            }
        };
        /**
        * @brief 获取设置多次的参数列表
        * @param out 输出参数列表
        */
        template <typename T>
        inline void getMultiVector(std::vector< std::vector<T> > &out)
        {
            std::stringstream ss;

            if (!isSet) {
                if (!defaults.empty()) {
                    std::vector< std::string > strings;
                    SplitDelim(defaults, delim, strings);

                    if (out.size() < 1) {
                        out.resize(1);
                    }

                    for (int i = 0; i < (long int)strings.size(); ++i) {
                        ss << strings[i];
                        T temp;
                        ss >> temp;
                        out[0].push_back(temp);
                        ss.str("");
                        ss.clear();
                    }
                }
            }
            else {
                if (!args.empty()) {
                    int n = args.size();

                    if ((long int)out.size() < n) {
                        out.resize(n);
                    }

                    for (int i = 0; i < n; ++i) {
                        for (int j = 0; j < (long int)args[i].size(); ++j) {
                            ss << args[i].at(j);
                            T temp;
                            ss >> temp;
                            out[i].push_back(temp);
                            ss.str("");
                            ss.clear();
                        }
                    }
                }
            }
        };
        /**
        * @brief 验证参数是否正确
        * @param badOptions 错误信息
        * @return 是否正确
        */
        inline bool validate(std::vector<std::string> &badOptions)
        {
            if (needValidate) {
                /* need Validate */
                /* check type */
                switch (optType) {
                case EZ_BOOL: {
                    bool example = true;
                    return validate(example, badOptions);
                }

                case EZ_INT8: {
                    char example = 0;
                    return validate(example, badOptions);
                }

                case EZ_UINT8: {
                    unsigned char example = 0;
                    return validate(example, badOptions);
                }

                case EZ_INT16: {
                    short example = 0;
                    return validate(example, badOptions);
                }

                case EZ_UINT16: {
                    unsigned short example = 0;
                    return validate(example, badOptions);
                }

                case EZ_INT32: {
                    int example = 0;
                    return validate(example, badOptions);
                }

                case EZ_UINT32: {
                    unsigned int example = 0;
                    return validate(example, badOptions);
                }

                case EZ_INT64: {
                    long example = 0;
                    return validate(example, badOptions);
                }

                case EZ_UINT64: {
                    unsigned long example = 0;
                    return validate(example, badOptions);
                }

                case EZ_FLOAT: {
                    float example = 0;
                    return validate(example, badOptions);
                }

                case EZ_DOUBLE: {
                    double example = 0;
                    return validate(example, badOptions);
                }

                case EZ_TEXT: {
                    std::string example = "";
                    return validate(example, badOptions);
                }

                case EZ_FILE: {
                    for (int i = 0; i < (int)args.size(); ++i) {
                        for (int j = 0; j < (int)args[i].size(); ++j) {
                            std::string arg = args[i].at(j);

                            if (!CheckFileExistence(arg)) {
                                badOptions.push_back(args[i].at(j));
                            }
                        }
                    }
                    break;
                }

                case EZ_DIR: {
                    for (int i = 0; i < (int)args.size(); ++i) {
                        for (int j = 0; j < (int)args[i].size(); ++j) {
                            std::string arg = args[i].at(j);

                            if (!CheckDirExistence(arg.c_str())) {
                                badOptions.push_back(args[i].at(j));
                            }
                        }
                    }
                    break;
                }

                case EZ_NOTYPE:
                default:
                    break;
                }
            }

            return badOptions.empty();
        };
    private:
        /**
        * @brief 验证参数是否正确,仅内部调用
        * @param example 数据类型示例，用于判断类型最大最小值
        * @param badOptions 错误信息
        * @return 是否正确
        */
        template<typename T>
        inline bool validate(T example,
            std::vector<std::string> &badOptions)
        {
            std::stringstream ss;

            if (validValues.size() == 0) {
                /*check range*/
                T mint = example, maxt = example;

                if (!minValue.empty()) {
                    ss << minValue;
                    ss >> mint;
                    ss.str("");
                    ss.clear();
                }
                else {
                    mint = std::numeric_limits<T>::min();
                }

                if (!maxValue.empty()) {
                    ss << maxValue;
                    ss >> maxt;
                    ss.str("");
                    ss.clear();
                }
                else {
                    maxt = std::numeric_limits<T>::max();
                }

                double min, max;
                ss << mint;
                ss >> min;
                ss.str("");
                ss.clear();
                ss.str("");
                ss << maxt;
                ss >> max;
                ss.str("");
                ss.clear();
                ss.str("");

                /*iterates all arguments*/
                for (int i = 0; i < (int)args.size(); ++i) {
                    for (int j = 0; j < (int)args[i].size(); ++j) {
                        ss << args[i].at(j);
                        double value;
                        ss >> value;
                        ss.str("");
                        ss.clear();
                        ss.str("");

                        if (value > max || value < min) {
                            badOptions.push_back(args[i].at(j));
                        }
                    }
                }
            }
            else {
                /* check arguments whether in the valid value list*/
                for (int i = 0; i < (int)args.size(); ++i) {
                    for (int j = 0; j < (int)args[i].size(); ++j) {
                        std::string arg = args[i].at(j);

                        if (std::find(validValues.begin(), validValues.end(), arg) == validValues.end()) {
                            badOptions.push_back(args[i].at(j));
                        }
                    }
                }
            }

            return badOptions.empty();
        };
    };

    /**
    * @brief 参数解析类
    */
    class OptionParser {
    public:
        OptionParser() : unlabeledNumber(0), xorGroupNum(0)
        {
            /* 添加help参数*/
            add("-h,--help,--usage", false, 0, "Print this usage message");
        };
        inline ~OptionParser() {};
        /**
        * @brief 添加参数
        * @param flags 参数标识,可有多个,get时使用,用逗号分隔
        没有带-或者--的,作为无标签的参数
        传入时不需要带前导符,比如input,可以直接写"input",传的时候直接传文件名
        * @param required 是否必须填写
        * @param optType 参数类型
        * @param expectArgs 期望传入数量
        0表示不带参数,
        -1表示任意多个,
        1表示一个,2表示两个,以此类推
        参数紧跟标志后，用逗号分隔)
        * @param help 帮助信息,最终显示在help中
        * @param defaults 默认值
        * @param minValue 最小值,若不需要则为""
        * @param maxValue 最大值,若不需要则为""
        * @param validListStr 有效值列表,设置后最大最小值无效,用逗号分隔,可以是数字或者单词,不需要则不指定或为""
        */
        inline void add(const char *flags,
            bool required = true,
            int expectArgs = 1,
            const char *help = "",
            EZ_TYPE optType = EZ_NOTYPE,
            const char *defaults = "",
            const std::string &minValue = std::string(),
            const std::string &maxValue = std::string(),
            const  char *validListStr = "")
        {
            int id = this->groups.size();
            OptionGroup g;
            g.defaults = defaults;
            g.isRequired = required;
            g.expectArgs = expectArgs;
            g.isSet = 0;

            if (optType != EZ_NOTYPE) {
                g.help.append("[").append(EZ_TYPE_NAME[optType]).append("] ");
            }

            g.help.append(help);
            std::vector<std::string> flagsVector;
            SplitDelim(flags, delim, flagsVector);
            g.flags = flagsVector;
            g.optType = optType;
            g.minValue = minValue;
            g.maxValue = maxValue;
            std::vector<std::string> validList;
            SplitDelim(validListStr, delim, validList);
            g.validValues = validList;

            if (optType == EZ_TEXT && validList.size() > 0) {
                g.needValidate = true;
            }
            else if (optType != EZ_TEXT && optType != EZ_NOTYPE) {
                g.needValidate = true;
            }

            if (flagsVector[0].substr(0, 1) != "-") {
                /*reset some properties*/
                g.isUnlabeled = true;
                g.isRequired = true;/*option must be set if defined as unlabeled*/
                unlabeledPos.insert(std::pair<int, std::string>(unlabeledNumber, flagsVector[0]));
                unlabeledNumber++;
                g.argsFormat.append("(");
            }

            if (g.expectArgs == 0) {
                g.argsFormat.append(" ");
            }
            else if (g.expectArgs == -1) {
                g.argsFormat.append(" Arg0,[ArgN] ");
            }
            else if (g.expectArgs == 1) {
                if (g.validValues.size() == 0) {
                    g.argsFormat.append(" Arg ");
                }
                else {
                    g.argsFormat.append(" ");

                    for (int k = 0; k < (int)g.validValues.size(); ++k) {
                        g.argsFormat.append(g.validValues[k]).append("|");
                    }
                }
            }
            else {
                std::stringstream ss;
                g.argsFormat.append(" ");

                for (int i = 0; i < g.expectArgs; i++) {
                    std::string argstr;
                    ss << i;
                    ss >> argstr;
                    ss.str("");
                    ss.clear();
                    g.argsFormat.append("Arg").append(argstr).append(",");
                }
            }

            g.argsFormat.erase(g.argsFormat.size() - 1);

            if (g.isUnlabeled) {
                g.argsFormat.append(" )");
            }

            for (int i = 0; i < (int)flagsVector.size(); ++i) {
                this->optionGroupIds[flagsVector[i]] = id;
            }

            this->groups.push_back(g);
        };
        /**
        * @brief 添加互斥选项(开关只能同时开启一个),互斥选项必须为可选，否则将出现逻辑问题
        * @param xorlistStr 互斥选项列表,必须使用标识的第一个flag,用逗号分隔 opt.xorAdd("-d,-s");
        */
        inline void xorAdd(const char *xorlistStr)
        {
            xorGroupNum++;
            std::vector<std::string> list;
            SplitDelim(xorlistStr, delim, list);

            for (unsigned int i = 0; i < list.size(); i++) {
                if (optionGroupIds.count(list[i])) {
                    int Id = optionGroupIds[list[i]];
                    groups[Id].groupID = xorGroupNum;
                    xorGroups[xorGroupNum].push_back(Id);
                }
            }
        };
        /**
        * @brief 获取参数,使用任意flag都可
        * @param name 参数标识名
        * @return 参数
        */
        inline OptionGroup get(const char *name)
        {
            if (optionGroupIds.count(name)) {
                return groups[optionGroupIds[name]];
            }

            return OptionGroup();
        };
        /**
        * @brief 获取使用说明
        * @return 获取使用说明
        */
        inline std::string getUsage()
        {
            std::string usage;
            usage.append("NAME: \n\n    ")\
                .append(overview)\
                .append("\n\n")\
                .append("USAGE: \n\n    ");
            int i, j;

            if (syntax.empty()) {
                syntax.append(_progName).append(" ");
                std::vector<std::string> xorGroupUsage;

                for (i = 0; i < xorGroupNum; i++) {
                    xorGroupUsage.push_back("");
                }

                for (i = 0; i < (int)groups.size(); i++) {
                    OptionGroup g = groups[i];
                    std::string opt = "";

                    if (g.groupID == 0) {
                        /*no xor*/
                        if (!g.isRequired) {
                            opt = "[";
                        }

                        opt.append(g.flags[0]).append(g.argsFormat);

                        if (!g.isRequired) {
                            opt.append("]");
                        }

                        opt.append(" ");
                    }
                    else {
                        int groupID = g.groupID;

                        if (xorGroupUsage[groupID - 1].empty()) {
                            xorGroupUsage[groupID - 1].append(" [");
                        }

                        xorGroupUsage[groupID - 1].append(g.flags[0])\
                            .append(g.argsFormat)\
                            .append(" | ");
                    }

                    syntax.append(opt);
                }

                for (i = 0; i < xorGroupNum; i++) {
                    xorGroupUsage[i].erase(xorGroupUsage[i].size() - 3);
                    xorGroupUsage[i].append("] ");
                    syntax.append(xorGroupUsage[i]);
                }
            }

            usage.append(syntax).append("\n\nOPTIONS:\n\n");

            for (i = 0; i < (int)groups.size(); i++) {
                OptionGroup g = groups[i];
                usage.append("    ");
                std::string flags;

                for (j = 0; j < (int)g.flags.size(); ++j) {
                    flags.append(g.flags[j]).append(" ");
                }

                usage.append(flags)\
                    .append(g.argsFormat)\
                    .append(":\n")\
                    .append("        ")\
                    .append(g.help)\
                    .append("\n");
            }

            if (!example.empty()) {
                usage.append("\n\nEXAMPLES:\n\n    ").append(example);
            }

            if (!footer.empty()) {
                usage.append("\n\nNotes:\n\n    ").append(footer).append("\n");
            }

            return usage;
        };
        /**
        * @brief 判断group是否设置
        * @param name 标识名
        * @return 1为已设置,0为未设置
        */
        inline int isSet(const char *name)
        {
            std::string sname(name);

            if (this->optionGroupIds.count(sname)) {
                return this->groups[this->optionGroupIds[sname]].isSet;
            }

            return 0;
        };
        /**
        * @brief 解析参数,直接将argc argv传入
        */
        inline bool parse(int argc, const char *const *argv)
        {
            if (argc < 1) {
                std::cout << "Invalid optoin number" << std::endl;
                return false;
            }

            std::vector<std::string> args;
            int i, j, Id, unlabel = 0;

            for (i = 0; i < (int)argc; i++) {
                args.push_back(argv[i]);
            }

            _progName = args.front();
            args.erase(args.begin());
            std::string s;

            for (i = 0; i < (int)args.size(); i++) {
                s = args[i];

                if (s.substr(0, 1) == "-" && s.substr(0, 2) != "--") {
                    /*args with - , maybe combined*/
                    s = s.substr(1);
                    int length = s.size();

                    for (j = 0; j < length; ++j) {
                        std::string letter = s.substr(j, 1);
                        letter = "-" + letter;

                        if (optionGroupIds.count(letter)) {
                            Id = optionGroupIds[letter];
                            groups[Id].isSet = 1;

                            if (groups[Id].expectArgs) {
                                ++i;

                                if (i >= (int)args.size()) {
                                    break;
                                }

                                std::vector<std::string> argOptions;
                                SplitDelim(args[i], delim, argOptions);
                                groups[Id].args.push_back(argOptions);
                            }
                        }
                        else if (std::find(unknownOptions.begin(), \
                            unknownOptions.end(), \
                            letter) == unknownOptions.end()) {
                            unknownOptions.push_back(letter);
                        }
                    }
                }
                else if (s.substr(0, 2) == "--") {
                    /*long name single option*/
                    if (optionGroupIds.count(s)) {
                        Id = optionGroupIds[s];
                        groups[Id].isSet = 1;

                        if (groups[Id].expectArgs) {
                            ++i;

                            if (i >= (int)args.size()) {
                                break;
                            }

                            std::vector<std::string> argOptions;
                            SplitDelim(args[i], delim, argOptions);
                            groups[Id].args.push_back(argOptions);
                        }
                    }
                    else if (std::find(unknownOptions.begin(), \
                        unknownOptions.end(), \
                        s) == unknownOptions.end()) {
                        unknownOptions.push_back(s);
                    }
                }
                else {
                    /*unlabeled OptionGroup*/
                    if (unlabeledPos.count(unlabel)) {
                        s = unlabeledPos[unlabel];

                        if (optionGroupIds.count(s)) {
                            Id = optionGroupIds[s];
                            groups[Id].isSet = 1;
                            std::vector<std::string> argOptions;
                            SplitDelim(args[i], delim, argOptions);
                            groups[Id].args.push_back(argOptions);
                        }

                        unlabel++;
                    }
                    else if (std::find(unknownOptions.begin(), \
                        unknownOptions.end(), \
                        s) == unknownOptions.end()) {
                        unknownOptions.push_back(s);
                    }
                }
            }

            if (isSet("-h")) {
                std::cout << getUsage() << std::endl;
                return false;
            }

            std::string out;

            if (!checkValid(out)) {
                std::cout << out << std::endl;
                return false;
            }

            //显示警告
            std::cout << out << std::endl;
            return true;
        };

        /**
        * @brief 检查输入参数格式是否正确
        * @param out 输出提示信息
        * @return 输入参数格式是否正确
        */
        inline bool checkValid(std::string &out)
        {
            int i, j;
            bool isValid = true;

            /*check required*/
            for (i = 0; i < (int)groups.size(); ++i) {
                if (groups[i].isRequired && !groups[i].isSet) {
                    out.append("ERROR: Argument ")\
                        .append(groups[i].flags[0]).append(" must be set.\n");
                    isValid = false;
                }
            }

            /*check expectArgs*/
            for (i = 0; i < (int)groups.size(); ++i) {
                OptionGroup g = groups[i];

                if (g.isSet) {
                    if (g.expectArgs != 0 && g.args.empty()) {
                        out.append("ERROR: Got unexpected number of arguments for option ") \
                            .append(g.flags[0]).append(".\n");
                        isValid = false;
                        continue;
                    }

                    for (j = 0; j < (int)g.args.size(); ++j) {
                        if ((g.expectArgs != -1 && g.expectArgs != (int)g.args[j].size())
                            || (g.expectArgs == -1 && g.args[j].size() == 0)) {
                            out.append("ERROR: Got unexpected number of arguments for option ") \
                                .append(g.flags[0]).append(".\n");
                            isValid = false;
                        }
                    }
                }
            }

            /*check arguments validation*/
            std::vector<std::string> tempOptions;

            for (i = 0; i < (int)groups.size(); ++i) {
                if (!groups[i].validate(tempOptions)) {
                    for (j = 0; j < (int)tempOptions.size(); ++j) {
                        out.append("ERROR: Got invalid argument \"") \
                            .append(tempOptions[j]).append("\" for option ") \
                            .append(groups[i].flags[0]).append(".\n");
                        isValid = false;
                    }
                }

                tempOptions.clear();
            }

            /*check xor groups*/

            for (std::map<int, std::vector<int> >::iterator it =
                xorGroups.begin(); it != xorGroups.end(); ++it) {
                std::vector<int> xorIDs = it->second;
                std::string xorstr;
                int xorNum = 0;

                for (i = 0; i < (int)xorIDs.size(); i++) {
                    if (groups[xorIDs[i]].isSet) {
                        xorstr.append(" \"") \
                            .append(groups[xorIDs[i]].flags[0]).append("\" ");
                        xorNum++;
                    }
                }

                if (xorNum > 1) {
                    out.append("ERROR:Can't set xor options at the same time:") \
                        .append(xorstr).append("\n");
                    isValid = false;
                }
            }

            /*print unknown OPTIONS*/
            if ((int)unknownOptions.size() > 0) {
                for (i = 0; i < (int)unknownOptions.size(); ++i) {
                    out.append("WARNING: Got unknown argument:") \
                        .append(unknownOptions[i]).append(".\n");
                }
            }

            /*return out*/
            if (!out.empty() && !isValid) {
                out.append("\n").append(getUsage());
            }

            return isValid;
        };
        /*description*/
        std::string overview;  /** @brief 命令行工具作用简要说明  */
                               /** @brief 命令行语法（可自动生成，如无必要可不比改）  */
        std::string syntax;
        std::string example;  /** @brief 示例  */
        std::string footer;  /** @brief 脚注，说明license、作者等信息  */
    private:
        int unlabeledNumber;
        std::string _progName;  /** @brief 工具名称，直接从argv中解析得到  */
                                /*members*/
                                /** @brief 参数标签哈希 key=>参数 value=>id   */
        std::map< std::string, int > optionGroupIds;
        std::vector<OptionGroup> groups;  /** @brief 参数组  */
                                          /** @brief 互斥参数组哈希,key=>groupID,value=>参数ID列表  */
        std::map<int, std::vector<int> > xorGroups;
        std::vector<std::string> unknownOptions;  /** @brief 无标签参数组  */
        int xorGroupNum;  /** @brief xorGroup 总数  */
                          /** @brief 无标签参数组名称位置对应表  */
        std::map<int, std::string> unlabeledPos;
    };
}
/* ################################################################### */
#endif /* EZ_OPTION_PARSER_H */