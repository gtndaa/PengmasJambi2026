const { jwt } = require('jsonwebtoken');
const authenticateJWT = (req, res, next) => {
 const token = req.headers['Authorization'];
 //const token = authHeader && authHeader.split(' ')[1];
 //const token = req.header('Authorization');
 if (!token) {
 return res.status(401).json({ message: 'Access denied' });
 }
 try {
 const decoded = jwt.verify(token, process.env.ACCESS_TOKEN_SECRET);
 req.user = decoded;
 next();
 } catch (error) {
 res.status(400).json({ message: `Invalid token ${token}` });
 }
};
module.exports = { authenticateJWT };
