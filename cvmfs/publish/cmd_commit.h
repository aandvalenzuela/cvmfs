#include <string>
#include <vector>

#include "publish/command.h"

namespace publish {

class CmdCommit : public Command {
 public:

  virtual std::string GetName() const { return "commit"; }

  virtual std::string GetBrief() const {
    return "Commit changes of a transaction";
  }

  virtual std::string GetDescription() const {
    return "This is a first implementation of the commit command";
  }

  virtual std::string GetUsage() const {
    return "[options] <repository name>[path]";
  }

  virtual ParameterList GetParams() const {
    ParameterList p;
    p.push_back(Parameter::Optional("commit-param", 'c', "commit",
      "Possible parameter for commit command"
      "commit param"));
    return p;
  }

  virtual std::vector<std::string> GetExamples() const {
    std::vector<std::string> e;
    e.push_back(
        "-c"
        "commit example");
    return e;
  }

  virtual int Main(const Options &options);

}; 

} // namespace publish
