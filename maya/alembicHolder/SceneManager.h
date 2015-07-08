#ifndef _AlembicHolder_SceneManager_h_
#define _AlembicHolder_SceneManager_h_

#include "Foundation.h"
#include "Drawable.h"
#include <map>
#include <utility>
#include <string>
#include <vector>
#include <fstream>

#include <boost/shared_ptr.hpp>

namespace AlembicHolder {

typedef std::pair<AlembicHolder::ScenePtr, unsigned int> CountedScene;

class SceneManager
{
    public:
        void addScene(std::string abcFile, std::string objectPath) {
            std::fstream file;
            file.open(abcFile.c_str());
            if (file.is_open())
            {
                 //the file exists
                file.close();

                std::string key = abcFile+"|"+objectPath;
    //            std::cout << "addScene: " << key << std::endl;
                try {
                    if (m_scenes.count(key)) {
                        m_scenes[key].second++;
    //                std::cout << "add incr " << std::endl;
                    } else {
                        CountedScene cs( AlembicHolder::ScenePtr( new AlembicHolder::Scene(abcFile,objectPath) ), 1);
                        m_scenes[key] = cs;
                    }
                }
                catch (...)
                {
                    if (m_scenes.count(key)) {
                        m_scenes.erase(key);
    //            std::cout << "catch " << std::endl;
                    }
                }
            }
            else {
                file.close();
                std::cout << "[nozAlembicHolder] Can't open file: " << abcFile << std::endl;
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
