#pragma once
#include <vector>
#include <string>
#include <functional>
#include <memory>

#define InfiniteRange std::numeric_limits<uint64_t>::max()
typedef std::shared_ptr<class IHttpRequest> HttpRequestPtr;
typedef std::shared_ptr<class IHttpResponse> HttpResponsePtr;

typedef std::function<void(HttpRequestPtr, HttpResponsePtr, bool)> HttpRequestCompleteDelegateType;
typedef std::function<void(HttpRequestPtr, int64_t, int64_t, int64_t)> HttpRequestProgressDelegateType;

enum EHttpRequestStatus
{
    NotStarted,
    Processing,
    Failed,
    Failed_ConnectionError,
    Succeeded
};

typedef struct MimePart {
    std::string Name;
    std::string Data;
    std::string FileName;
    std::string FileData;

}MimePart_t;

typedef struct InMimePart {
    std::string_view Name;
    std::string_view Data;
    std::string_view FileName;
    std::string_view FileData;
}InMimePart_t;


class IHttpBase
{
public:

    /**
     * Get the URL used to send the request.
     *
     * @return the URL string.
     */
    virtual std::string_view GetURL() = 0;

    /**
     * Gets an URL parameter.
     * expected format is ?Key=Value&Key=Value...
     * If that format is not used, this function will not work.
     *
     * @param ParameterName - the parameter to request.
     * @return the parameter value string.
     */
    virtual std::string_view GetURLParameter(const std::string_view ParameterName) = 0;

    /**
     * Gets the value of a header, or empty string if not found.
     *
     * @param HeaderName - name of the header to set.
     */
    virtual std::string_view GetHeader(const std::string_view HeaderName) = 0;

    /**
     * Return all headers in an array in "Name: Value" format.
     *
     * @return the header array of strings
     */
    virtual std::vector<std::string> GetAllHeaders() = 0;

    /**
     * Shortcut to get the Content-Type header value (if available)
     *
     * @return the content type.
     */
    virtual std::string_view GetContentType() = 0;

    /**
     * Shortcut to get the Content-Length header value. Will not always return non-zero.
     * If you want the real length of the payload, get the payload and check it's length.
     *
     * @return the content length (if available)
     */
    virtual int64_t GetContentLength() = 0;

    /**
     * Get the content payload of the request or response.
     *
     * @param Content - array that will be filled with the content.
     */
    virtual const std::vector<uint8_t>& GetContent() = 0;

    virtual std::vector<MimePart_t> GetAllMime() = 0;
    /**
     * Destructor for overrides
     */
    virtual ~IHttpBase() {};
};


class IHttpRequest :
    public IHttpBase, public std::enable_shared_from_this<IHttpRequest>
{

public:
    /**
     * Gets the verb (GET, PUT, POST) used by the request.
     *
     * @return the verb string
     */
    virtual std::string_view GetVerb() = 0;

    /**
     * Sets the verb used by the request.
     * Eg. (GET, PUT, POST)
     * Should be set before calling ProcessRequest.
     * If not specified then a GET is assumed.
     *
     * @param Verb - verb to use.
     */
    virtual void SetVerb(const std::string_view Verb) = 0;

    /**
     * Sets the URL for the request
     * Eg. (http://my.domain.com/something.ext?key=value&key2=value).
     * Must be set before calling ProcessRequest.
     *
     * @param URL - URL to use.
     */
    virtual void SetURL(const std::string_view URL) = 0;

    virtual void SetHost(const std::string_view Host) = 0;
    virtual void SetPath(const std::string_view Path) = 0;
    virtual void SetScheme(const std::string_view Scheme) = 0;
    virtual void SetPortNum(uint32_t port) = 0;
    virtual void SetQuery(const std::string_view QueryName, const std::string_view QueryValue) = 0;
    virtual void SetRange(uint64_t begin, uint64_t end = InfiniteRange) = 0;
    /**
     * Sets the content of the request (optional data).
     * Usually only set for POST requests.
     *
     * @param ContentPayload - payload to set.
     */
    virtual void SetContent(const std::vector<uint8_t>& ContentPayload) = 0;

    /**
     * Sets the content of the request as a string encoded as UTF8.
     *
     * @param ContentString - payload to set.
     */
    virtual void SetContentAsString(const std::string_view ContentString) = 0;

    /**
     * Sets optional header info.
     * SetHeader for a given HeaderName will overwrite any previous values
     * Use AppendToHeader to append more values for the same header
     * Content-Length is the only header set for you.
     * Required headers depends on the request itself.
     * Eg. "multipart/form-data" needed for a form post
     *
     * @param HeaderName - Name of the header (ie, Content-Type)
     * @param HeaderValue - Value of the header
     */
    virtual void SetHeader(const std::string_view HeaderName, const std::string_view HeaderValue) = 0;

    /**
    * Appends to the value already set in the header.
    * If there is already content in that header, a comma delimiter is used.
    * If the header is as of yet unset, the result is the same as calling SetHeader
    * Content-Length is the only header set for you.
    * Also see: SetHeader()
    *
    * @param HeaderName - Name of the header (ie, Content-Type)
    * @param AdditionalHeaderValue - Value to add to the existing contents of the specified header.
    *	comma is inserted between old value and new value, per HTTP specifications
    */
    virtual void AppendToHeader(const std::string_view HeaderName, const std::string_view AdditionalHeaderValue) = 0;

    /**
    * Set Mime Data(ie form-data)
    */
    virtual void SetMimePart(InMimePart_t part) = 0;
    /**
     * Called to begin processing the request.
     * OnProcessRequestComplete delegate is always called when the request completes or on error if it is bound.
     * A request can be re-used but not while still being processed.
     *
     * @return if the request was successfully started.
     */
    virtual bool ProcessRequest() = 0;

    /**
     * Delegate called when the request is complete. See FHttpRequestCompleteDelegate
     */
    virtual HttpRequestCompleteDelegateType& OnProcessRequestComplete() = 0;

    /**
     * Delegate called to update the request/response progress. See FHttpRequestProgressDelegate
     */
    virtual HttpRequestProgressDelegateType& OnRequestProgress() = 0;

    /**
     * Called to cancel a request that is still being processed
     */
    virtual void CancelRequest() = 0;


    /**
     * Get the current status of the request being processed
     *
     * @return the current status
     */
    virtual EHttpRequestStatus GetStatus() = 0;

    /**
     * Get the associated Response
     *
     * @return the response
     */
    virtual const HttpResponsePtr GetResponse() const = 0;

    /**
     * Used to tick the request
     *
     * @param DeltaSeconds - seconds since last ticked
     */
    virtual void Tick(float DeltaSeconds) = 0;

    /**
     * Gets the time that it took for the server to fully respond to the request.
     *
     * @return elapsed time in seconds.
     */
    virtual float GetElapsedTime() = 0;

    /**
     * Destructor for overrides
     */
    virtual ~IHttpRequest() {};
};


class IHttpResponse : public IHttpBase
{
public:

    /**
     * Gets the response code returned by the requested server.
     * See EHttpResponseCodes for known response codes
     *
     * @return the response code.
     */
    virtual int32_t GetResponseCode() = 0;

    /**
     * Returns the payload as a string, assuming the payload is UTF8.
     *
     * @return the payload as a string.
     */
    virtual std::string_view GetContentAsString() = 0;

    /**
    * Get payload length not the content-length header.
    */
    virtual int64_t GetContentBytesRead() = 0;

    /**
    * Use user buf in response.
    * After set GetContentAsString and GetContent will return empty vector.
    * If payload larger than buf. the extra data will be discard.
    */
    virtual void SetContentBuf(void* ptr, int64_t len) = 0;
    /**
     * Destructor for overrides
     */
    virtual ~IHttpResponse() {};
};


