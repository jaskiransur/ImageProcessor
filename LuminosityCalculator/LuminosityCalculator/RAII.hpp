#ifndef raii_hpp
template<typename PredicateCreate, typename PredicateDestroy, typename Resource, typename... Args>
struct RAII
{
	/*! takes a resource and multiple number of arguments (0 or more) argumets
	   precondition: The resource is already constructor, and the creator merely applies a predicate on it.
	   The predicate may take an argument or none at all.
	*/
	RAII(Resource& data, Args&& ...args)
		:data_(data)
	{
		PredicateCreate creator;
		creator(data, std::forward<Args>(args)...);
	}

	/*! woraround for a tempResource& 
	takes a resource and multiple number of arguments (0 or more) argumets
	   precondition: The resource is already constructed, and the creator merely applies a predicate on it.
	   The predicate may take an argument or none at all.
	*/
	RAII(Resource* data, Args&& ...args)
		:data_(data)
	{
		PredicateCreate creator;
		creator(data, std::forward<Args>(args)...);
	}
	/*! takes a resource and multiple number of arguments (0 or more) argumets
	   precondition: The resource is already constructor, and the creator merely applies a predicate on it.
	   The predicate may take an argument or none at all.
	*/
	RAII(Resource&& data, Args&& ...args)
		:data_(std::forward<Resource>(data))
	{
		PredicateCreate creator;
		creator(data, std::forward<Args>(args)...);
	}

	/*! creates a resource with default argument
	   or with more than one argument
	   precondition: The resource must have a default constructor implemented
	   It may have more than constructor taking ...args (variable arguments)
	*/
	RAII(Args&& ...args)
	{
		PredicateCreate creator;
		data_ = creator(std::forward<Args>(args)...);
	}

	RAII(const RAII& other) = delete;
	RAII& operator=(RAII& other) = delete;

	/*! move constructor 
	*/
	RAII(RAII&& other)
		:data_(std::move(other.data_))
	{

	}

	/*! move assignement operator 
	*/
	RAII& operator=(RAII&& other)
	{
		data_ = std::move(other.data_);
		return *this
	}

	/*! destructor, which also calls the predicate to cleanup 
	*   the resource
	*/
	~RAII()
	{
		PredicateDestroy destroyer;
		destroyer(data_);
	}


	/*! returns a const reference to resource
	*/
	const Resource& get() const
	{
		return data_;
	}


	/*! returns a reference to resource
	*/
	Resource& get()
	{
		return data_;
	}
private:
	Resource data_;
};


#endif // !raii_hpp
