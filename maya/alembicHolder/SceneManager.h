#ifndef _AlembicHolder_SceneManager_h_
#define _AlembicHolder_SceneManager_h_

#include "Foundation.h"
#include "Drawable.h"
#include <map>
#include <utility>
#include <string>
#include <vector>
#include <fstream>

namespace AlembicHolder {

typedef std::pair<AlembicHolder::ScenePtr, unsigned int> CountedScene;

class SceneManager
{
    public:
        void addScene(std::vector<std::string> abcFiles, std::string objectPath) 
        {
            std::string key;
            std::fstream file;
            bool allIsGood = true;
            for (size_t i = 0; i < abcFiles.size(); i++)
            {
                file.open(abcFiles[i].c_str(), std::ios_base::in);
                if (file.is_open())
                {
                    //the file exists
                    key += abcFiles[i] + "|";
                }
                else
                {
                    
                    std::cout << "[nozAlembicHolder] Can't open file: " << abcFiles[i] << std::endl;
                    allIsGood = false;
                }
                file.close();
            }

            if (allIsGood == false)
                return;


            key += objectPath;

            try 
            {
                if (m_scenes.count(key)) {
                    m_scenes[key].second++;
//                std::cout << "add incr " << std::endl;
                } else {
                    CountedScene cs( AlembicHolder::ScenePtr( new AlembicHolder::Scene(abcFiles,objectPath) ), 1);
                    m_scenes[key] = cs;
                }
            }
            catch (...)
            {
                if (m_scenes.count(key)) 
                    m_scenes.erase(key);
            }
        }

        void addScene(std::string key, AlembicHolder::ScenePtr scene_ptr) {
            if (m_scenes.count(key)) { m_scenes[key].second++; }
            else { m_scenes[key] = CountedScene(scene_ptr, 1); }
        }

        AlembicHolder::ScenePtr getScene(std::string key) {
            return m_scenes[key].first; }

        void removeScene(std::string key) {
            if (m_scenes.count(key)) {
                if (m_scenes[key].second > 1) {
                    m_scenes[key].second--;
//                    std::cout << key << " - " << m_scenes[key].second << " instances left!" << std::endl;
                } else {
                    m_scenes.erase(key);
                    std::cout << "[nozAlembicHolder] Closed: " << key << std::endl;

//                    std::cout << key << " - last instance removed" << std::endl;
                }
            }
        }

        unsigned int hasKey(std::string key) { return m_scenes.count(key); }

    private:
        std::map<std::string, CountedScene> m_scenes;

};

} // End namespace AlembicHolder

#endif
