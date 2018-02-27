classdef (Abstract) BaseClass < handle
%mexcpp.BaseClass   Base Matlab class to wrap C++ class instance
%
%   An abstract base class to wrap C++ backend class instance. The user backend
%   C++ class shall inherit mexFunctionClass derived in include/mexObjectHandler.h.
%   This Matlab base class serves to provide a concrete foundation to interact
%   with the C++ class, implementing common functions such as backend object
%   construction and destruction, set/get backend object properties, and
%   save/load backend data.
%
%   The header file 'include/mexObjectHandler.h' defines 3 key elements to interact
%   with this Matlab class: mexObjectHandle, mexObjectHandler, and mexFunctionClass.
%
%   The mexObjectHandle is based on Oliver Woodford's class wrapper
%   (https://www.mathworks.com/matlabcentral/fileexchange/38964), which provides
%   the way to store C++ pointer as a Matlab uint64 variable.
%
%   The mexObjectHandler template function requires to be specified for the user's
%   C++ class should be the only line in the user's mexFunction(), receiving all
%   the arguments as is. This function accepts the following Matlab syntax:
%
%      backend = mexfcn(obj)                        - backend construction
%      mexfcn(obj,'delete')                         - backend destruction
%      varargout = mexfcn(obj, 'command', varargin) - backend action
%      varargout = mexfcn('command',varargin)       - backend static action
%      mexfcn(obj, 'set', 'name', value)            - backend property set
%      value = mexfcn(obj, 'get', 'name', value)    - backend property get
%      data = mexfcn(obj, 'save')                   - backend data save
%      mexfcn(obj, 'load', data)                    - backend data load
%
%   Note 'delete', 'set', 'get', 'save', 'load' are the reserved action commands.
%
%   See also
   
   properties (Access = protected, Hidden, NonCopyable, Transient)
      backend % Handle to the backend C++ class instance
   end
   
   methods (Access = protected, Static, Hidden, Abstract)
      varargout = mexfcn(varargin) % mex function, defined by derived class
   end
   
   methods
      function obj = BaseClass(varargin)
         % instantiate mex backend
         obj.mexfcn(obj, varargin{:});
      end
      
      function delete(obj)
         % destroy the mex backend
         if ~isempty(obj.backend)
            obj.mexfcn(obj, 'delete');
         end
      end
   end
end
