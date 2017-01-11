#ifndef __vtkIGTLIOCommandMessageCodec_h_
#define __vtkIGTLIOCommandMessageCodec_h_

#include <vector>
#include <utility>

class vtkIGTLIOCommandMessageCodec
{

public:

    vtkIGTLIOCommandMessageCodec();
    vtkIGTLIOCommandMessageCodec( bool isReply );

    // Build the content of the command to send
    void SetResult( bool res );
    void SetParameter( std::string paramName, std::string value );
    std::string GetContent();

    // Parse the content of a command received
    void SetContent( std::string content );
    int GetNumberOfParameters() { return m_parameters.size(); }
    bool IsReply() { return m_isReply; }
    bool GetResult();
    std::string GetParameter( std::string paramName );

protected:

    typedef std::vector<std::pair<std::string,std::string> > ParamContainer;
    bool m_isReply;
    bool m_result;
    ParamContainer m_parameters;
};

#endif